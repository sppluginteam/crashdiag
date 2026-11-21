cmake_minimum_required(VERSION 3.16)
project(crashdiag LANGUAGES CXX)

# =============================================================================
# crashdiag - Source engine server crash-dump diagnostic plugin
#
# Engine-level plugin exposing ISERVERPLUGINCALLBACKS003. Captures fatal
# exceptions, writes a text report, a minidump (.mdmp) and a classification
# (.cause) file, and periodically appends a heartbeat.log line.
#
# Build modes:
#
#   CRASHDIAG_STUB_SDK=ON  (default)
#       Compile against the bundled stub headers in sdk/stub/. No HL2SDK
#       checkout needed; this is the CI type-check path and it links a real
#       shared object because the stub SDK also stubs tier0/ConVar symbols.
#
#   CRASHDIAG_STUB_SDK=OFF -DHL2SDK_PATH=<dir>
#       Compile against a real HL2SDK (e.g. hl2sdk-dods).
#
#       CRASHDIAG_COMPILE_ONLY=ON (recommended for CI)
#           Build an OBJECT library only: verifies that the sources are
#           header/ABI compatible with the real SDK without linking. This is
#           necessary because hl2sdk ships headers only - tier0/tier1
#           libraries must be built separately (as SourceMod does), so a
#           link against the SDK alone always ends in LNK2019.
#
#       CRASHDIAG_COMPILE_ONLY=OFF (requires real tier0/tier1 libs)
#           Produces the loadable .dll / .so. Point the build at a tree where
#           tier0.lib/tier1.lib are available.
# =============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

option(CRASHDIAG_STUB_SDK "Compile against the bundled stub SDK" ON)
option(CRASHDIAG_COMPILE_ONLY "Object library only (no link) - real-SDK header check" OFF)
set(HL2SDK_PATH "" CACHE PATH "Path to a real HL2SDK public/ tree (non-stub build)")

set(CRASHDIAG_SOURCES
    "${SRC_DIR}/plugin.cpp"
    "${SRC_DIR}/crash_report.cpp"
    "${SRC_DIR}/heartbeat.cpp"
    "${SRC_DIR}/interface.cpp"
)

if(CRASHDIAG_COMPILE_ONLY)
    add_library(crashdiag OBJECT ${CRASHDIAG_SOURCES})
else()
    add_library(crashdiag SHARED ${CRASHDIAG_SOURCES})
endif()

if(CRASHDIAG_STUB_SDK)
    target_compile_definitions(crashdiag PRIVATE CRASHDIAG_STUB_SDK)
    target_include_directories(crashdiag PRIVATE
        "${SRC_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/sdk/stub")
    # Stub implementations of the ConVar/ConCommand/tier0 symbols.
    target_sources(crashdiag PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sdk/stub/stub_sdk.cpp")
else()
    if(HL2SDK_PATH STREQUAL "")
        message(FATAL_ERROR
            "Non-stub build needs -DHL2SDK_PATH=<path to hl2sdk public/ tree>.")
    endif()
    target_include_directories(crashdiag PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${SRC_DIR}"
        "${HL2SDK_PATH}/public"
        "${HL2SDK_PATH}/public/tier0"
        "${HL2SDK_PATH}/public/tier1"
        "${HL2SDK_PATH}/tier0"
        "${HL2SDK_PATH}/tier1"
        "${HL2SDK_PATH}/common")
endif()

target_compile_definitions(crashdiag PRIVATE
    _CRT_SECURE_NO_WARNINGS
    WIN32_LEAN_AND_MEAN
    NOMINMAX)

# hl2sdk's public/tier0/platform.h reasons about COMPILER_MSVC being pre-defined
# (its build system used to pass it) and errors out at VALIGNOF without it:
#   #error "PORT: Code only tested with MSVC! ..."
if(MSVC)
    target_compile_definitions(crashdiag PRIVATE COMPILER_MSVC=1)
endif()

if(NOT CRASHDIAG_COMPILE_ONLY)
    if(WIN32)
        target_link_libraries(crashdiag PRIVATE dbghelp psapi)
        set_target_properties(crashdiag PROPERTIES
            OUTPUT_NAME "crashdiag"
            PREFIX ""
            SUFFIX ".dll")
    else()
        set_target_properties(crashdiag PROPERTIES
            OUTPUT_NAME "crashdiag"
            PREFIX "lib"
            SUFFIX ".so")
    endif()
endif()

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
#       Compile against a real HL2SDK (e.g. hl2sdk-dods). This links a real
#       dll/so and needs neither tier0.lib nor tier1.lib: the plugin carries its
#       own interface factory and its own ConVar/ConCommand bodies
#       (src/compat/tier1_min.cpp) instead of linking Valve's libraries.
#
#       CRASHDIAG_COMPILE_ONLY=ON
#           Build an OBJECT library instead of a shared one, for header/ABI
#           checks only.
# =============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

option(CRASHDIAG_STUB_SDK "Compile against the bundled stub SDK" ON)
option(CRASHDIAG_COMPILE_ONLY "Object library only (no link)" OFF)
set(HL2SDK_PATH "" CACHE PATH "Path to a real HL2SDK public/ tree (non-stub build)")

set(CRASHDIAG_SOURCES
    "${SRC_DIR}/plugin.cpp"
    "${SRC_DIR}/crash_report.cpp"
    "${SRC_DIR}/heartbeat.cpp"
    "${SRC_DIR}/interface.cpp"
    "${SRC_DIR}/log.cpp"
)

# Bodies for the public ConVar/ConCommand classes: hl2sdk ships no tier1.lib, so
# the real-SDK build supplies them locally (see the file header).
if(NOT CRASHDIAG_STUB_SDK)
    list(APPEND CRASHDIAG_SOURCES "${SRC_DIR}/compat/tier1_min.cpp")
endif()

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
        "${HL2SDK_PATH}/public/../common")
endif()

target_compile_definitions(crashdiag PRIVATE
    _CRT_SECURE_NO_WARNINGS
    WIN32_LEAN_AND_MEAN
    NOMINMAX)

# Valve's public headers expect the macros its old VPC build system used to
# define on the command line. Without them the SDK hits internal #errors:
#   platform.h     #error "PORT: Code only tested with MSVC!"      (COMPILER_MSVC)
#   threadtools.h  #error "implement me"                          (COMPILER_MSVC32/64)
#   threadtools.h  #error "Interlocked functions not defined."     (same)
# The 32/64 split also drives the PAUSE and interlocked implementations.
if(MSVC)
    target_compile_definitions(crashdiag PRIVATE COMPILER_MSVC=1)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_compile_definitions(crashdiag PRIVATE COMPILER_MSVC64=1)
    else()
        target_compile_definitions(crashdiag PRIVATE COMPILER_MSVC32=1)
    endif()
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

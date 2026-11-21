cmake_minimum_required(VERSION 3.16)
project(crashdiag LANGUAGES CXX)

# =============================================================================
# crashdiag - Source engine server crash-dump diagnostic plugin
#
# Engine-level plugin exposing ISERVERPLUGINCALLBACKS003. Captures fatal
# exceptions, writes a text report, a minidump (.mdmp) and a classification
# (.cause) file, and periodically appends a heartbeat.log line.
#
# Two build modes:
#
#   CRASHDIAG_STUB_SDK=ON  (default)
#       Compile against the bundled stub headers in sdk/stub/. No HL2SDK
#       checkout is needed; this is what the CI syntax check uses.
#
#   CRASHDIAG_STUB_SDK=OFF -DHL2SDK_PATH=<dir>
#       Compile against a real HL2SDK (e.g. hl2sdk-dods). <dir> must contain
#       the SDK's public/ header tree (eiface.h, icvar.h, interface.h, ...).
# =============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

option(CRASHDIAG_STUB_SDK "Compile against the bundled stub SDK" ON)
set(HL2SDK_PATH "" CACHE PATH "Path to a real HL2SDK public/ tree (non-stub build)")

set(CRASHDIAG_SOURCES
    "${SRC_DIR}/plugin.cpp"
    "${SRC_DIR}/crash_report.cpp"
    "${SRC_DIR}/heartbeat.cpp"
    "${SRC_DIR}/interface.cpp"
)

add_library(crashdiag SHARED ${CRASHDIAG_SOURCES})

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

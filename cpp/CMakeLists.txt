cmake_minimum_required(VERSION 3.16)
project(MedicatInstaller VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Pick 7za for target CPU architecture.
if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
    set(BUNDLE_7ZA "${REPO_ROOT}/7z/arm64/7za.exe")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    if(EXISTS "${REPO_ROOT}/7z/x64/7za.exe")
        set(BUNDLE_7ZA "${REPO_ROOT}/7z/x64/7za.exe")
    else()
        set(BUNDLE_7ZA "${REPO_ROOT}/7za.exe")
    endif()
else()
    set(BUNDLE_7ZA "${REPO_ROOT}/7z/x32/7za.exe")
endif()

set(BUNDLE_7Z "${REPO_ROOT}/bin/7z.exe")

if(NOT EXISTS "${BUNDLE_7ZA}")
    message(FATAL_ERROR "7za.exe not found for bundling: ${BUNDLE_7ZA}")
endif()
if(NOT EXISTS "${BUNDLE_7Z}")
    message(FATAL_ERROR "7z.exe not found for bundling: ${BUNDLE_7Z}")
endif()

file(TO_NATIVE_PATH "${BUNDLE_7ZA}" BUNDLE_7ZA_PATH)
file(TO_NATIVE_PATH "${BUNDLE_7Z}" BUNDLE_7Z_PATH)
# RC treats \7 as an escape — use forward slashes in the resource script.
string(REPLACE "\\" "/" BUNDLE_7ZA_PATH "${BUNDLE_7ZA_PATH}")
string(REPLACE "\\" "/" BUNDLE_7Z_PATH "${BUNDLE_7Z_PATH}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/bundle.rc.in" "${CMAKE_BINARY_DIR}/bundle.rc" @ONLY)

add_executable(MedicatInstaller WIN32
    src/main.cpp
    src/app.cpp
    src/gui.cpp
    src/drives.cpp
    src/extract.cpp
    src/ventoy.cpp
    src/util.cpp
    src/log.cpp
    src/bundle.cpp
    "${CMAKE_BINARY_DIR}/bundle.rc"
)

target_include_directories(MedicatInstaller PRIVATE src "${CMAKE_CURRENT_SOURCE_DIR}/res")

if(MSVC)
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /I\"${CMAKE_CURRENT_SOURCE_DIR}/res\"")
endif()

target_compile_definitions(MedicatInstaller PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    MEDICAT_VERSION="21.12"
)

target_link_libraries(MedicatInstaller PRIVATE
    comctl32
    shell32
    shlwapi
    user32
    gdi32
    ole32
    uuid
)

if(MSVC)
    target_compile_options(MedicatInstaller PRIVATE /W4 /permissive-)
    target_link_options(MedicatInstaller PRIVATE
        "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'"
    )
endif()

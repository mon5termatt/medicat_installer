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
set(BUNDLE_MD5 "${REPO_ROOT}/MedicatFiles.md5")

if(NOT EXISTS "${BUNDLE_7ZA}")
    message(FATAL_ERROR "7za.exe not found for bundling: ${BUNDLE_7ZA}")
endif()
if(NOT EXISTS "${BUNDLE_7Z}")
    message(FATAL_ERROR "7z.exe not found for bundling: ${BUNDLE_7Z}")
endif()
if(NOT EXISTS "${BUNDLE_MD5}")
    message(FATAL_ERROR "MedicatFiles.md5 not found for bundling: ${BUNDLE_MD5}")
endif()

file(TO_NATIVE_PATH "${BUNDLE_7ZA}" BUNDLE_7ZA_PATH)
file(TO_NATIVE_PATH "${BUNDLE_7Z}" BUNDLE_7Z_PATH)
file(TO_NATIVE_PATH "${BUNDLE_MD5}" BUNDLE_MD5_PATH)
# RC treats \7 as an escape — use forward slashes in the resource script.
string(REPLACE "\\" "/" BUNDLE_7ZA_PATH "${BUNDLE_7ZA_PATH}")
string(REPLACE "\\" "/" BUNDLE_7Z_PATH "${BUNDLE_7Z_PATH}")
string(REPLACE "\\" "/" BUNDLE_MD5_PATH "${BUNDLE_MD5_PATH}")

set(VENTOY_VERSIONS_GEN "${CMAKE_BINARY_DIR}/ventoy_versions.txt")
set(VENTOY_VERSIONS_SCRIPT "${REPO_ROOT}/tools/fetch_ventoy_versions.py")
set(VENTOY_VERSIONS_FALLBACK "${CMAKE_CURRENT_SOURCE_DIR}/res/ventoy_versions.txt")

add_custom_command(
    OUTPUT "${VENTOY_VERSIONS_GEN}"
    COMMAND python "${VENTOY_VERSIONS_SCRIPT}" "${VENTOY_VERSIONS_GEN}" --fallback "${VENTOY_VERSIONS_FALLBACK}"
    DEPENDS "${VENTOY_VERSIONS_SCRIPT}" "${VENTOY_VERSIONS_FALLBACK}"
    COMMENT "Fetching Ventoy version list for embedding"
    WORKING_DIRECTORY "${REPO_ROOT}"
)

file(TO_NATIVE_PATH "${VENTOY_VERSIONS_GEN}" VENTOY_VERSIONS_PATH)
string(REPLACE "\\" "/" VENTOY_VERSIONS_PATH "${VENTOY_VERSIONS_PATH}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/bundle.rc.in" "${CMAKE_BINARY_DIR}/bundle.rc" @ONLY)
set_source_files_properties("${CMAKE_BINARY_DIR}/bundle.rc" PROPERTIES
    OBJECT_DEPENDS "${VENTOY_VERSIONS_GEN};${BUNDLE_MD5}"
)

set(I18N_JSON "${REPO_ROOT}/i18n/translations.json")
set(I18N_GEN "${CMAKE_CURRENT_SOURCE_DIR}/src/i18n_generated.h")
set(I18N_CODEGEN "${REPO_ROOT}/tools/i18n_codegen.py")

add_custom_command(
    OUTPUT "${I18N_GEN}"
    COMMAND python "${I18N_CODEGEN}"
    DEPENDS "${I18N_CODEGEN}" "${I18N_JSON}"
    COMMENT "Generating i18n_generated.h"
    WORKING_DIRECTORY "${REPO_ROOT}"
)

add_executable(MedicatInstaller WIN32
    src/main.cpp
    src/app.cpp
    src/gui.cpp
    src/drives.cpp
    src/extract.cpp
    src/ventoy.cpp
    src/download.cpp
    src/util.cpp
    src/log.cpp
    src/bundle.cpp
    src/i18n.cpp
    src/theme.cpp
    src/verify.cpp
    src/offline.cpp
    src/debug.cpp
    "${CMAKE_BINARY_DIR}/bundle.rc"
    "${CMAKE_BINARY_DIR}/ventoy_versions.txt"
    "${I18N_GEN}"
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
    INSTALLER_VERSION="${PROJECT_VERSION}"
    MEDICAT_USB_VERSION="21.12"
)

target_link_libraries(MedicatInstaller PRIVATE
    comctl32
    shell32
    shlwapi
    user32
    gdi32
    ole32
    uuid
    winhttp
    windowscodecs
    dwmapi
    uxtheme
    gdiplus
    advapi32
)

if(MSVC)
    target_compile_options(MedicatInstaller PRIVATE /W4 /permissive- /utf-8)
    target_link_options(MedicatInstaller PRIVATE
        "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'"
    )
endif()

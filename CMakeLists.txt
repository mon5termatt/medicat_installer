cmake_minimum_required(VERSION 3.16)
project(MedicatInstaller VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# Bundle 7za matching the installer target CPU (x64 or Win32).
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BUNDLE_7ZA "${REPO_ROOT}/bin/7z/x64/7za.exe")
    set(ARIA2_ARCH "x64")
else()
    set(BUNDLE_7ZA "${REPO_ROOT}/bin/7z/x32/7za.exe")
    set(ARIA2_ARCH "x32")
endif()
set(BUNDLE_MD5 "${REPO_ROOT}/MedicatFiles.md5")
set(BUNDLE_MD5_GZ "${REPO_ROOT}/generated/MedicatFiles.md5.gz")
set(PREPARE_MD5_SCRIPT "${REPO_ROOT}/tools/prepare_md5_bundle.py")
set(PREPARE_ARIA2_SCRIPT "${REPO_ROOT}/tools/prepare_aria2_bundle.py")
set(BUNDLE_ARIA2_EXE "${REPO_ROOT}/bin/aria2/${ARIA2_ARCH}/aria2c.exe")
set(BUNDLE_ARIA2_GZ "${CMAKE_BINARY_DIR}/aria2c.exe.gz")

if(NOT EXISTS "${BUNDLE_7ZA}")
    message(FATAL_ERROR "7za.exe not found for bundling: ${BUNDLE_7ZA}")
endif()
if(NOT EXISTS "${BUNDLE_MD5}")
    message(FATAL_ERROR "MedicatFiles.md5 not found for bundling: ${BUNDLE_MD5}")
endif()

add_custom_command(
    OUTPUT "${BUNDLE_MD5_GZ}"
    COMMAND python "${PREPARE_MD5_SCRIPT}" "${BUNDLE_MD5}" "${BUNDLE_MD5_GZ}"
    DEPENDS "${PREPARE_MD5_SCRIPT}" "${BUNDLE_MD5}"
    COMMENT "Preparing compressed MD5 manifest for bundling"
    WORKING_DIRECTORY "${REPO_ROOT}"
)

add_custom_command(
    OUTPUT "${BUNDLE_ARIA2_GZ}"
    COMMAND python "${PREPARE_ARIA2_SCRIPT}"
            --arch "${ARIA2_ARCH}"
            --cache-exe "${BUNDLE_ARIA2_EXE}"
            --output "${BUNDLE_ARIA2_GZ}"
    DEPENDS "${PREPARE_ARIA2_SCRIPT}"
    COMMENT "Downloading and compressing aria2c for bundling (${ARIA2_ARCH})"
    WORKING_DIRECTORY "${REPO_ROOT}"
)

file(TO_NATIVE_PATH "${BUNDLE_7ZA}" BUNDLE_7ZA_PATH)
file(TO_NATIVE_PATH "${BUNDLE_MD5_GZ}" BUNDLE_MD5_GZ_PATH)
file(TO_NATIVE_PATH "${BUNDLE_ARIA2_GZ}" BUNDLE_ARIA2_GZ_PATH)
# RC treats \7 as an escape — use forward slashes in the resource script.
string(REPLACE "\\" "/" BUNDLE_7ZA_PATH "${BUNDLE_7ZA_PATH}")
string(REPLACE "\\" "/" BUNDLE_MD5_GZ_PATH "${BUNDLE_MD5_GZ_PATH}")
string(REPLACE "\\" "/" BUNDLE_ARIA2_GZ_PATH "${BUNDLE_ARIA2_GZ_PATH}")

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

set(DISCORD_ICON_SCRIPT "${REPO_ROOT}/tools/prepare_discord_icon.py")
set(DISCORD_PNG "${CMAKE_CURRENT_SOURCE_DIR}/res/discord.png")
set(DISCORD_ICO "${CMAKE_CURRENT_SOURCE_DIR}/res/discord.ico")

add_custom_command(
    OUTPUT "${DISCORD_ICO}"
    COMMAND python "${DISCORD_ICON_SCRIPT}"
    DEPENDS "${DISCORD_ICON_SCRIPT}" "${DISCORD_PNG}"
    COMMENT "Building Discord icon from res/discord.png"
    WORKING_DIRECTORY "${REPO_ROOT}"
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/bundle.rc.in" "${CMAKE_BINARY_DIR}/bundle.rc" @ONLY)
set_source_files_properties("${CMAKE_BINARY_DIR}/bundle.rc" PROPERTIES
    OBJECT_DEPENDS "${VENTOY_VERSIONS_GEN};${BUNDLE_MD5_GZ};${BUNDLE_7ZA};${BUNDLE_ARIA2_GZ};${DISCORD_ICO}"
)

set(BUILD_NUMBER_FILE "${REPO_ROOT}/build_number.txt")
set(BUILD_VERSION_CPP "${REPO_ROOT}/generated/build_version.cpp")
set(INGEST_TOKEN_CPP "${REPO_ROOT}/generated/ingest_token.cpp")
set(GENERATE_INGEST_SCRIPT "${REPO_ROOT}/tools/generate_ingest_token.py")
file(MAKE_DIRECTORY "${REPO_ROOT}/generated")
if(NOT EXISTS "${BUILD_VERSION_CPP}")
    file(WRITE "${BUILD_VERSION_CPP}"
"// Placeholder — regenerated before each build.\nnamespace medicat {\nextern const char kInstallerVersion[] = \"0.0.0\";\nextern const int kInstallerBuildNumber = 0;\n}\n"
    )
endif()
if(NOT EXISTS "${INGEST_TOKEN_CPP}")
    file(WRITE "${INGEST_TOKEN_CPP}"
"// Placeholder — regenerated at configure time.\n#include \"ingest_token.h\"\nnamespace medicat {\nstd::string GetIngestToken() { return {}; }\nbool HasIngestToken() { return false; }\n}\n"
    )
endif()

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

# This function makes it so when a new code file is added, the project
#   is reconfigured automatically (iirc)
set_property(
  DIRECTORY APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS
  "src/*.cpp"
)
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.hpp")
add_executable(MedicatInstaller WIN32
    ${SOURCE_FILES}
    "${BUILD_VERSION_CPP}"
    "${INGEST_TOKEN_CPP}"
    "${CMAKE_BINARY_DIR}/bundle.rc"
    "${CMAKE_BINARY_DIR}/ventoy_versions.txt"
    "${BUNDLE_MD5_GZ}"
    "${BUNDLE_ARIA2_GZ}"
    "${I18N_GEN}"
    "${DISCORD_ICO}"
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set_target_properties(MedicatInstaller PROPERTIES OUTPUT_NAME "MedicatInstaller-x86")
endif()

set(_PIN_BUILD "")
if(DEFINED ENV{MEDICAT_PIN_BUILD} AND NOT "$ENV{MEDICAT_PIN_BUILD}" STREQUAL "")
    set(_PIN_BUILD "$ENV{MEDICAT_PIN_BUILD}")
elseif(DEFINED MEDICAT_PIN_BUILD AND NOT "${MEDICAT_PIN_BUILD}" STREQUAL "")
    set(_PIN_BUILD "${MEDICAT_PIN_BUILD}")
endif()
set(_SYNC_BUILD_NUMBER_CMD
    python "${REPO_ROOT}/tools/bump_build_number.py"
    "${BUILD_NUMBER_FILE}"
    "${BUILD_VERSION_CPP}"
    --major ${PROJECT_VERSION_MAJOR}
    --minor ${PROJECT_VERSION_MINOR}
    --skip-if-env MEDICAT_BUILD_NUMBER_BUMPED
    --keep
)
if(NOT _PIN_BUILD STREQUAL "")
    list(APPEND _SYNC_BUILD_NUMBER_CMD --set "${_PIN_BUILD}")
endif()
add_custom_command(TARGET MedicatInstaller PRE_BUILD
    COMMAND ${_SYNC_BUILD_NUMBER_CMD}
    WORKING_DIRECTORY "${REPO_ROOT}"
    COMMENT "Syncing embedded build number"
)

target_include_directories(MedicatInstaller PRIVATE src "${CMAKE_CURRENT_SOURCE_DIR}/res")

if(MSVC)
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /I\"${CMAKE_CURRENT_SOURCE_DIR}/res\"")
endif()

# Production default; optional cmake/local.cmake for dev overrides (see cmake/local.cmake.example).
set(MEDICAT_SESSIONS_URL "https://telemetry.medicatusb.com/v1/sessions")
set(MEDICAT_UPLOADS_URL "https://telemetry.medicatusb.com/v1/support/uploads")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/local.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/local.cmake")
endif()

set(MEDICAT_INGEST_TOKEN_VALUE "")
if(DEFINED MEDICAT_INGEST_TOKEN)
    set(MEDICAT_INGEST_TOKEN_VALUE "${MEDICAT_INGEST_TOKEN}")
elseif(DEFINED ENV{MEDICAT_INGEST_TOKEN})
    set(MEDICAT_INGEST_TOKEN_VALUE "$ENV{MEDICAT_INGEST_TOKEN}")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ingest.token")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ingest.token" MEDICAT_INGEST_TOKEN_VALUE)
    string(STRIP "${MEDICAT_INGEST_TOKEN_VALUE}" MEDICAT_INGEST_TOKEN_VALUE)
endif()

set(_INGEST_TOKEN_DEPS "${GENERATE_INGEST_SCRIPT}")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ingest.token")
    list(APPEND _INGEST_TOKEN_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ingest.token")
endif()

add_custom_command(
    OUTPUT "${INGEST_TOKEN_CPP}"
    COMMAND python "${GENERATE_INGEST_SCRIPT}" "${MEDICAT_INGEST_TOKEN_VALUE}" "${INGEST_TOKEN_CPP}"
    DEPENDS ${_INGEST_TOKEN_DEPS}
    COMMENT "Generating obfuscated ingest token"
    WORKING_DIRECTORY "${REPO_ROOT}"
)

set(INSTALLER_RELEASE_TAG "unknown")
# Release GitHub tag matches installer version (1.0.N). Prefer build_number.txt.
if(EXISTS "${BUILD_NUMBER_FILE}")
    file(READ "${BUILD_NUMBER_FILE}" INSTALLER_RELEASE_TAG)
    string(STRIP "${INSTALLER_RELEASE_TAG}" INSTALLER_RELEASE_TAG)
elseif(EXISTS "${REPO_ROOT}/release_tag.txt")
    file(READ "${REPO_ROOT}/release_tag.txt" INSTALLER_RELEASE_TAG)
    string(STRIP "${INSTALLER_RELEASE_TAG}" INSTALLER_RELEASE_TAG)
endif()

target_compile_definitions(MedicatInstaller PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    MEDICAT_USB_VERSION="21.12"
    MEDICAT_SESSIONS_URL="${MEDICAT_SESSIONS_URL}"
    MEDICAT_UPLOADS_URL="${MEDICAT_UPLOADS_URL}"
    INSTALLER_RELEASE_TAG="${INSTALLER_RELEASE_TAG}"
    MEDICAT_RELEASE_TAG="${INSTALLER_RELEASE_TAG}"
)

target_link_libraries(MedicatInstaller PRIVATE
    comctl32
    comdlg32
    shell32
    shlwapi
    user32
    gdi32
    ole32
    uuid
    winhttp
    rpcrt4
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

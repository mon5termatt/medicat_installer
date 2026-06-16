cmake_minimum_required(VERSION 3.16)
project(MedicatInstaller VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

include("cmake/7zip.cmake")
include("cmake/ventoy.cmake")

#region i18n generation
set(I18N_JSON "${REPO_ROOT}/i18n/translations.json")
set(I18N_GEN "${CMAKE_CURRENT_SOURCE_DIR}/src/i18n_generated.hpp")
set(I18N_CODEGEN "${REPO_ROOT}/tools/i18n_codegen.py")

add_custom_command(
    OUTPUT "${I18N_GEN}"
    COMMAND python "${I18N_CODEGEN}"
    DEPENDS "${I18N_CODEGEN}" "${I18N_JSON}"
    COMMENT "Generating i18n_generated.h"
    WORKING_DIRECTORY "${REPO_ROOT}"
)
#endregion

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

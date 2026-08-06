# Unified superbuild: one configure + one `cmake --build` produces x64 and Win32 installers.
cmake_minimum_required(VERSION 3.16)
project(MedicatInstallerUnified NONE)

set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")

if(NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    message(FATAL_ERROR "Unified build requires a Visual Studio generator (Visual Studio 17 2022 or newer).")
endif()

include(ExternalProject)

set(_COMMON_CMAKE_CACHE_ARGS
    -DCMAKE_GENERATOR:STRING=${CMAKE_GENERATOR}
)

if(DEFINED ENV{MEDICAT_BUILD_NUMBER_BUMPED})
    list(APPEND _COMMON_CMAKE_CACHE_ARGS
        -DMEDICAT_BUILD_NUMBER_BUMPED:STRING=$ENV{MEDICAT_BUILD_NUMBER_BUMPED})
endif()

ExternalProject_Add(medicat_installer_x64
    SOURCE_DIR "${REPO_ROOT}"
    BINARY_DIR "${CMAKE_BINARY_DIR}/x64"
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_GENERATOR_PLATFORM x64
    CMAKE_CACHE_ARGS ${_COMMON_CMAKE_CACHE_ARGS}
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
    INSTALL_COMMAND ""
    BUILD_ALWAYS TRUE
    USES_TERMINAL_BUILD TRUE
)

ExternalProject_Add(medicat_installer_x86
    SOURCE_DIR "${REPO_ROOT}"
    BINARY_DIR "${CMAKE_BINARY_DIR}/x86"
    CMAKE_GENERATOR "${CMAKE_GENERATOR}"
    CMAKE_GENERATOR_PLATFORM Win32
    CMAKE_CACHE_ARGS ${_COMMON_CMAKE_CACHE_ARGS}
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
    INSTALL_COMMAND ""
    BUILD_ALWAYS TRUE
    USES_TERMINAL_BUILD TRUE
)

set(_STAGED_X64 "${CMAKE_BINARY_DIR}/Release/MedicatInstaller.exe")
set(_STAGED_X86 "${CMAKE_BINARY_DIR}/Release/MedicatInstaller-x86.exe")
set(_BUILT_X64 "${CMAKE_BINARY_DIR}/x64/Release/MedicatInstaller.exe")
set(_BUILT_X86 "${CMAKE_BINARY_DIR}/x86/Release/MedicatInstaller-x86.exe")

add_custom_command(
    OUTPUT "${_STAGED_X64}"
    DEPENDS medicat_installer_x64
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/Release"
    COMMAND ${CMAKE_COMMAND} -E copy "${_BUILT_X64}" "${_STAGED_X64}"
    COMMENT "Staging x64 MedicatInstaller.exe"
)

add_custom_command(
    OUTPUT "${_STAGED_X86}"
    DEPENDS medicat_installer_x86
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/Release"
    COMMAND ${CMAKE_COMMAND} -E copy "${_BUILT_X86}" "${_STAGED_X86}"
    COMMENT "Staging Win32 MedicatInstaller-x86.exe"
)

add_custom_target(MedicatInstallerAll
    DEPENDS "${_STAGED_X64}" "${_STAGED_X86}"
)

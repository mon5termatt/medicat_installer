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

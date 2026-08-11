# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

set(SBOM_PROJECT_NAME "AttributionFiles")
_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "${SBOM_PROJECT_NAME}"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "1.0.0"
)
sbom_test_record_project()

# The attribution file has two entries. The first one has a PURL and a CPE with a $<VERSION>
# placeholder, the second one has neither, but has a different version. The second entry is
# processed in a nested _qt_internal_sbom_add_target call, which must not inherit the
# attribution values of the parent entry.
_qt_internal_add_sbom(AttributionMultiEntry
    SBOM_ENTITY_TYPE THIRD_PARTY_SOURCES
    USE_ATTRIBUTION_FILES
    ATTRIBUTION_FILE_DIR_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/attributions/multi_entry"
)

# The first entry's own values.
add_assert_str_exists_in_spdx_v2_3_doc(
    "ExternalRef: PACKAGE-MANAGER purl pkg:github/sbomtest/parent@1\\.2\\.3")
add_assert_str_exists_in_spdx_v2_3_doc(
    "ExternalRef: PACKAGE-MANAGER purl pkg:generic/sbomtest/parent@1\\.2\\.3")
add_assert_str_exists_in_spdx_v2_3_doc(
    "ExternalRef: SECURITY cpe23Type cpe:2\\.3:a:sbomtest:parent:1\\.2\\.3:")
add_assert_str_exists_in_cydx_v1_6_doc("pkg:generic/sbomtest/parent@1\\.2\\.3")
add_assert_str_exists_in_cydx_v1_6_doc("pkg:github/sbomtest/parent@1\\.2\\.3")
add_assert_str_exists_in_cydx_v1_6_doc("cpe:2\\.3:a:sbomtest:parent:1\\.2\\.3:")

# The second entry has to show up as a separate package.
add_assert_str_exists_in_spdx_v2_3_doc("AttributionMultiEntry_Attribution_sbom-test-child")

# The second entry must not contain the first entry's PURL and CPE (modulo its own version).
add_assert_str_not_exists_in_spdx_v2_3_doc("pkg:github/sbomtest/parent@9\\.9\\.9")
add_assert_str_not_exists_in_spdx_v2_3_doc("pkg:generic/sbomtest/parent@9\\.9\\.9")
add_assert_str_not_exists_in_spdx_v2_3_doc("cpe:2\\.3:a:sbomtest:parent:9\\.9\\.9:")
add_assert_str_not_exists_in_cydx_v1_6_doc("pkg:github/sbomtest/parent@9\\.9\\.9")
add_assert_str_not_exists_in_cydx_v1_6_doc("pkg:generic/sbomtest/parent@9\\.9\\.9")
add_assert_str_not_exists_in_cydx_v1_6_doc("cpe:2\\.3:a:sbomtest:parent:9\\.9\\.9:")

# Check presence of copyrights as a drive by.
add_assert_str_exists_in_spdx_v2_3_doc("2026 Sbom Test Parent Authors")
add_assert_str_exists_in_spdx_v2_3_doc("2026 Sbom Test Parent Contributors")

_qt_internal_sbom_end_project()

sbom_test_end()

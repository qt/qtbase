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

# The attribution file has three entries.
# Selecting one via ATTRIBUTION_ENTRY_INDEX must process only that entry, both when the
# values are applied to the target itself, and when a nested sbom target is created
# for the entry (CREATE_SBOM_FOR_EACH_ATTRIBUTION).
# An out of range index is a configure error, see attribution_entry_index_out_of_range.cmake.
set(entry_index_attribution_dir "${CMAKE_CURRENT_SOURCE_DIR}/attributions/entry_index")

# Asserts that the package of the given target has the given version.
function(add_assert_entry_index_target_version target version)
    string(REPLACE "." "\\." version "${version}")
    string(CONCAT needle
        "PackageName: ${target}\n"
        "SPDXID:[^\n]*\n"
        "PackageDownloadLocation:[^\n]*\n"
        "PackageVersion: ${version}\n"
    )
    add_assert_str_exists_in_spdx_v2_3_doc("${needle}")
endfunction()

function(add_assert_entry_index_nested_target_exists target id)
    add_assert_str_exists_in_spdx_v2_3_doc("PackageName: ${target}_Attribution_${id}\n")
endfunction()

function(add_assert_entry_index_nested_target_not_exists target id)
    add_assert_str_not_exists_in_spdx_v2_3_doc("${target}_Attribution_${id}")
endfunction()

# No index. The first entry is applied to the target, the remaining entries get nested targets.
_qt_internal_add_sbom(EntryIndexSingle_none
    SBOM_ENTITY_TYPE THIRD_PARTY_SOURCES
    USE_ATTRIBUTION_FILES
    ATTRIBUTION_FILE_DIR_PATHS "${entry_index_attribution_dir}"
)
add_assert_entry_index_target_version(EntryIndexSingle_none "1.0.0")
add_assert_entry_index_nested_target_not_exists(EntryIndexSingle_none entry-index-first)
add_assert_entry_index_nested_target_exists(EntryIndexSingle_none entry-index-second)
add_assert_entry_index_nested_target_exists(EntryIndexSingle_none entry-index-third)

# No index. All entries get nested targets, the target itself takes no values from the file.
_qt_internal_add_sbom(EntryIndexEach_none
    SBOM_ENTITY_TYPE THIRD_PARTY_SOURCES
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_DIR_PATHS "${entry_index_attribution_dir}"
)
add_assert_entry_index_target_version(EntryIndexEach_none "unknown")
add_assert_entry_index_nested_target_exists(EntryIndexEach_none entry-index-first)
add_assert_entry_index_nested_target_exists(EntryIndexEach_none entry-index-second)
add_assert_entry_index_nested_target_exists(EntryIndexEach_none entry-index-third)

# Index 0. The entry is applied to the target, no nested targets are created.
_qt_internal_add_sbom(EntryIndexSingle_0
    SBOM_ENTITY_TYPE THIRD_PARTY_SOURCES
    USE_ATTRIBUTION_FILES
    ATTRIBUTION_FILE_DIR_PATHS "${entry_index_attribution_dir}"
    ATTRIBUTION_ENTRY_INDEX 0
)
add_assert_entry_index_target_version(EntryIndexSingle_0 "1.0.0")
add_assert_entry_index_nested_target_not_exists(EntryIndexSingle_0 entry-index-first)
add_assert_entry_index_nested_target_not_exists(EntryIndexSingle_0 entry-index-second)
add_assert_entry_index_nested_target_not_exists(EntryIndexSingle_0 entry-index-third)

# Index 0. Only the selected entry gets a nested target.
_qt_internal_add_sbom(EntryIndexEach_0
    SBOM_ENTITY_TYPE THIRD_PARTY_SOURCES
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_DIR_PATHS "${entry_index_attribution_dir}"
    ATTRIBUTION_ENTRY_INDEX 0
)
add_assert_entry_index_target_version(EntryIndexEach_0 "unknown")
add_assert_entry_index_nested_target_exists(EntryIndexEach_0 entry-index-first)
add_assert_entry_index_nested_target_not_exists(EntryIndexEach_0 entry-index-second)
add_assert_entry_index_nested_target_not_exists(EntryIndexEach_0 entry-index-third)

# Index 1.
_qt_internal_add_sbom(EntryIndexSingle_1
    SBOM_ENTITY_TYPE THIRD_PARTY_SOURCES
    USE_ATTRIBUTION_FILES
    ATTRIBUTION_FILE_DIR_PATHS "${entry_index_attribution_dir}"
    ATTRIBUTION_ENTRY_INDEX 1
)
add_assert_entry_index_target_version(EntryIndexSingle_1 "2.0.0")
add_assert_entry_index_nested_target_not_exists(EntryIndexSingle_1 entry-index-first)
add_assert_entry_index_nested_target_not_exists(EntryIndexSingle_1 entry-index-second)
add_assert_entry_index_nested_target_not_exists(EntryIndexSingle_1 entry-index-third)

_qt_internal_add_sbom(EntryIndexEach_1
    SBOM_ENTITY_TYPE THIRD_PARTY_SOURCES
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_DIR_PATHS "${entry_index_attribution_dir}"
    ATTRIBUTION_ENTRY_INDEX 1
)
add_assert_entry_index_target_version(EntryIndexEach_1 "unknown")
add_assert_entry_index_nested_target_not_exists(EntryIndexEach_1 entry-index-first)
add_assert_entry_index_nested_target_exists(EntryIndexEach_1 entry-index-second)
add_assert_entry_index_nested_target_not_exists(EntryIndexEach_1 entry-index-third)

# Index 2.
_qt_internal_add_sbom(EntryIndexSingle_2
    SBOM_ENTITY_TYPE THIRD_PARTY_SOURCES
    USE_ATTRIBUTION_FILES
    ATTRIBUTION_FILE_DIR_PATHS "${entry_index_attribution_dir}"
    ATTRIBUTION_ENTRY_INDEX 2
)
add_assert_entry_index_target_version(EntryIndexSingle_2 "3.0.0")
add_assert_entry_index_nested_target_not_exists(EntryIndexSingle_2 entry-index-first)
add_assert_entry_index_nested_target_not_exists(EntryIndexSingle_2 entry-index-second)
add_assert_entry_index_nested_target_not_exists(EntryIndexSingle_2 entry-index-third)

_qt_internal_add_sbom(EntryIndexEach_2
    SBOM_ENTITY_TYPE THIRD_PARTY_SOURCES
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_DIR_PATHS "${entry_index_attribution_dir}"
    ATTRIBUTION_ENTRY_INDEX 2
)
add_assert_entry_index_target_version(EntryIndexEach_2 "unknown")
add_assert_entry_index_nested_target_not_exists(EntryIndexEach_2 entry-index-first)
add_assert_entry_index_nested_target_not_exists(EntryIndexEach_2 entry-index-second)
add_assert_entry_index_nested_target_exists(EntryIndexEach_2 entry-index-third)

_qt_internal_sbom_end_project()

sbom_test_end()

# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

set(SBOM_PROJECT_NAME "AttributionEntryIndexOutOfRange")
_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "${SBOM_PROJECT_NAME}"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "1.0.0"
)
sbom_test_record_project()

# The attribution file has three entries, so index 3 is out of range and configuration must fail.
_qt_internal_add_sbom(EntryIndexOutOfRange
    SBOM_ENTITY_TYPE THIRD_PARTY_SOURCES
    USE_ATTRIBUTION_FILES
    ATTRIBUTION_FILE_DIR_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/attributions/entry_index"
    ATTRIBUTION_ENTRY_INDEX 3
)

_qt_internal_sbom_end_project()

sbom_test_end()

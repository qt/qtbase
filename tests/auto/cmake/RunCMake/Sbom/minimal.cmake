# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

# These variables are used by sbom_test_end().
set(SBOM_VERSION "1.0.0")

_qt_internal_sbom_begin_project(
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "${SBOM_VERSION}"
)

include(common_targets.cmake)

_qt_internal_sbom_end_project()

sbom_test_end()

# Also create separate sboms for some sibling projects under this subdir.
add_subdirectory(subprojects)

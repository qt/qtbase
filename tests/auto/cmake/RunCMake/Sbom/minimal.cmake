# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

_qt_internal_sbom_begin_project(
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "1.0.0"
)

include(common_targets.cmake)

_qt_internal_sbom_end_project()


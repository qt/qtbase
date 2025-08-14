# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "${PROJECT_NAME}Project"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    LICENSE_EXPRESSION "LGPL-3.0-only"
    COPYRIGHTS "2025 The Qt Company Ltd."
    INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}"
    INSTALL_SBOM_DIR "sbom"
    DOWNLOAD_LOCATION "https://download.qt.io/sbom"
    CPE "cpe:2.3:a:qt:qtprojecttest:1.0.0:*:*:*:*:*:*:*"
    VERSION "1.0.0"
)

include(common_targets.cmake)

_qt_internal_sbom_end_project()


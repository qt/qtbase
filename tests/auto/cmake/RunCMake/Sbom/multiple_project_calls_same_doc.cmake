# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

set(SBOM_VERSION "1.0.0")
set(SBOM_INSTALL_DIR "sbom")

_qt_internal_sbom_begin_project(
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "${SBOM_VERSION}"
)
sbom_test_record_project()

# Create a single sbom for all sibling projects under this subdir.
set(CREATE_SINGLE_SBOM TRUE)
add_subdirectory(subprojects)

add_assert_str_exists_in_spdx_v2_3_doc("PackageName: subproj1_helper")
add_assert_str_exists_in_spdx_v2_3_doc("PackageName: subproj2_helper")
add_assert_str_exists_in_spdx_v2_3_doc("PackageName: FancySystemLib")
add_assert_str_exists_in_spdx_v2_3_doc("PackageName: Threads")

_qt_internal_sbom_end_project()

sbom_test_end()

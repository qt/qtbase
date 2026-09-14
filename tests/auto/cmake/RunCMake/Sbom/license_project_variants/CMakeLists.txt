# The caller sets the LICENSE_VARIANT_ variables.
project(${LICENSE_VARIANT_NAME})

_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "${PROJECT_NAME}"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "1.0.0"
    INSTALL_SBOM_DIR "sbom_${PROJECT_NAME}"
    ${LICENSE_VARIANT_ARGS}
)
sbom_test_record_project()

add_assert_str_exists_in_spdx_v2_3_doc(
    "PackageLicenseConcluded: ${LICENSE_VARIANT_CONCLUDED}\nPackageLicenseDeclared: ${LICENSE_VARIANT_DECLARED}\n")

foreach(needle IN LISTS LICENSE_VARIANT_ABSENT_NEEDLES)
    add_assert_str_not_exists_in_spdx_v2_3_doc("${needle}")
endforeach()

_qt_internal_sbom_end_project()

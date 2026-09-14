# The caller sets the LICENSE_DEFAULTS_VARIANT_ variables.
project(${LICENSE_DEFAULTS_VARIANT_NAME})

_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "${PROJECT_NAME}"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "1.0.0"
    INSTALL_SBOM_DIR "sbom_${PROJECT_NAME}"
    DEFAULT_TARGET_LICENSE_CONCLUDED_EXPRESSION "Zlib"
    DEFAULT_TARGET_LICENSE_DECLARED_EXPRESSION "NTP"
    ${LICENSE_DEFAULTS_VARIANT_ARGS}
)
sbom_test_record_project()

_qt_internal_add_sbom(${PROJECT_NAME}Library
    SBOM_ENTITY_TYPE "LIBRARY"
)
assert_package_licenses(${PROJECT_NAME}Library
    CONCLUDED "${LICENSE_DEFAULTS_VARIANT_CONCLUDED}"
    DECLARED "${LICENSE_DEFAULTS_VARIANT_DECLARED}")

foreach(needle IN LISTS LICENSE_DEFAULTS_VARIANT_ABSENT_NEEDLES)
    add_assert_str_not_exists_in_spdx_v2_3_doc("${needle}")
endforeach()

_qt_internal_sbom_end_project()

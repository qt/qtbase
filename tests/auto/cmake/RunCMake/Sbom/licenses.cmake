# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

# Asserts the concluded and declared license recorded for the package of the given target.
function(assert_package_licenses target)
    set(opt_args "")
    set(single_args
        CONCLUDED
        DECLARED
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")

    if(NOT QT_GENERATE_SBOM OR NOT QT_SBOM_GENERATE_SPDX_V2)
        return()
    endif()

    _qt_internal_sbom_get_spdx_id_for_target(${target} spdx_id)

    # Match from the SPDXID line through the fixed package fields, so the check can't accidentally
    # match another package's licenses.
    string(CONCAT needle
        "SPDXID: ${spdx_id}\n"
        "PackageDownloadLocation:[^\n]*\n"
        "PackageVersion:[^\n]*\n"
        "PackageSupplier:[^\n]*\n"
        "PackageLicenseConcluded: ${arg_CONCLUDED}\n"
        "PackageLicenseDeclared: ${arg_DECLARED}\n"
    )
    add_assert_str_exists_in_spdx_v2_3_doc("${needle}")
endfunction()

set(SBOM_PROJECT_NAME "Licenses")
set(SBOM_INSTALL_DIR "sbom_licenses")

# The document that hosts the target and file level cases. The project package license options are
# checked in the separate documents below.
_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "${SBOM_PROJECT_NAME}"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "1.0.0"
    INSTALL_SBOM_DIR "${SBOM_INSTALL_DIR}"
)
sbom_test_record_project()

# Only the older option is given, the package gets a concluded license and no declared one.
_qt_internal_add_sbom(LicenseOldOption
    SBOM_ENTITY_TYPE "LIBRARY"
    LICENSE_EXPRESSION "MIT"
)
assert_package_licenses(LicenseOldOption CONCLUDED "MIT" DECLARED "NOASSERTION")

# The concluded option wins over the older option.
_qt_internal_add_sbom(LicenseConcludedOverridesOldOption
    SBOM_ENTITY_TYPE "LIBRARY"
    LICENSE_EXPRESSION "curl"
    LICENSE_CONCLUDED_EXPRESSION "ISC"
)
assert_package_licenses(LicenseConcludedOverridesOldOption CONCLUDED "ISC" DECLARED "NOASSERTION")

# Both expressions can be set independently.
_qt_internal_add_sbom(LicenseConcludedAndDeclared
    SBOM_ENTITY_TYPE "LIBRARY"
    LICENSE_CONCLUDED_EXPRESSION "BSD-3-Clause"
    LICENSE_DECLARED_EXPRESSION "Unlicense"
)
assert_package_licenses(LicenseConcludedAndDeclared
    CONCLUDED "BSD-3-Clause" DECLARED "Unlicense")

# A declared expression can be set without a concluded one.
_qt_internal_add_sbom(LicenseDeclaredOnly
    SBOM_ENTITY_TYPE "LIBRARY"
    LICENSE_DECLARED_EXPRESSION "NTP"
)
assert_package_licenses(LicenseDeclaredOnly CONCLUDED "NOASSERTION" DECLARED "NTP")

# An explicit concluded expression replaces the qt entity type expressions, including
# QT_LICENSE_ID and the directory scope variable. The target is finalized right away, so that the
# variable can be unset again afterwards.
set(QT_SBOM_LICENSE_EXPRESSION "Xnet")
_qt_internal_add_sbom(LicenseQtEntityExplicit
    SBOM_ENTITY_TYPE "QT_MODULE"
    __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_LICENSE
    QT_LICENSE_ID "BSD3"
    LICENSE_CONCLUDED_EXPRESSION "MIT"
    IMMEDIATE_FINALIZATION
)
assert_package_licenses(LicenseQtEntityExplicit CONCLUDED "MIT" DECLARED "MIT")

# The compatibility option is AND-ed with them instead.
_qt_internal_add_sbom(LicenseQtEntityCompat
    SBOM_ENTITY_TYPE "QT_MODULE"
    __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_LICENSE
    QT_LICENSE_ID "BSD3"
    LICENSE_EXPRESSION "ISC"
    IMMEDIATE_FINALIZATION
)
assert_package_licenses(LicenseQtEntityCompat
    CONCLUDED "\\(ISC\\) AND \\(\\(BSD-3-Clause\\) AND \\(Xnet\\)\\)"
    DECLARED "\\(ISC\\) AND \\(\\(BSD-3-Clause\\) AND \\(Xnet\\)\\)")
unset(QT_SBOM_LICENSE_EXPRESSION)

# An attribution file license is AND-ed with an explicit concluded expression, in contrast to the
# qt entity type expressions, which are replaced by it.
_qt_internal_add_sbom(LicenseAttribution
    SBOM_ENTITY_TYPE "QT_MODULE"
    __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_LICENSE
    USE_ATTRIBUTION_FILES
    ATTRIBUTION_FILE_DIR_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/attributions/multi_entry"
    ATTRIBUTION_IDS "sbom-test-parent"
    LICENSE_CONCLUDED_EXPRESSION "ISC"
)
assert_package_licenses(LicenseAttribution
    CONCLUDED "\\(ISC\\) AND \\(MIT\\)" DECLARED "\\(ISC\\) AND \\(MIT\\)")

_qt_internal_sbom_end_project()

# All three options, the new concluded one takes precedence over the compatibility alias.
set(LICENSE_VARIANT_NAME "LicenseConcludedOverridesCompat")
set(LICENSE_VARIANT_ARGS
    LICENSE_EXPRESSION "curl"
    PROJECT_LICENSE_CONCLUDED_EXPRESSION "0BSD"
    PROJECT_LICENSE_DECLARED_EXPRESSION "Zlib")
set(LICENSE_VARIANT_CONCLUDED "0BSD")
set(LICENSE_VARIANT_DECLARED "Zlib")
set(LICENSE_VARIANT_ABSENT_NEEDLES "PackageLicenseConcluded: curl")
add_subdirectory(license_project_variants license_concluded_overrides_compat)

# Only the compatibility option, its value is recorded in both fields.
set(LICENSE_VARIANT_NAME "LicenseCompatOnly")
set(LICENSE_VARIANT_ARGS LICENSE_EXPRESSION "MIT-0")
unset(LICENSE_VARIANT_ABSENT_NEEDLES)
set(LICENSE_VARIANT_CONCLUDED "MIT-0")
set(LICENSE_VARIANT_DECLARED "MIT-0")
add_subdirectory(license_project_variants license_compat_only)

# Only the new concluded option, the declared field stays empty.
set(LICENSE_VARIANT_NAME "LicenseConcludedOnly")
set(LICENSE_VARIANT_ARGS PROJECT_LICENSE_CONCLUDED_EXPRESSION "Vim")
set(LICENSE_VARIANT_CONCLUDED "Vim")
set(LICENSE_VARIANT_DECLARED "NOASSERTION")
add_subdirectory(license_project_variants license_concluded_only)

# Only the new declared option, the concluded field stays empty.
set(LICENSE_VARIANT_NAME "LicenseDeclaredOnlyProject")
set(LICENSE_VARIANT_ARGS PROJECT_LICENSE_DECLARED_EXPRESSION "W3C")
set(LICENSE_VARIANT_CONCLUDED "NOASSERTION")
set(LICENSE_VARIANT_DECLARED "W3C")
add_subdirectory(license_project_variants license_declared_only)

# No license options at all.
set(LICENSE_VARIANT_NAME "LicenseNoneAtAll")
set(LICENSE_VARIANT_ARGS "")
set(LICENSE_VARIANT_CONCLUDED "NOASSERTION")
set(LICENSE_VARIANT_DECLARED "NOASSERTION")
add_subdirectory(license_project_variants license_none_at_all)

sbom_test_end()

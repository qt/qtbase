# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

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

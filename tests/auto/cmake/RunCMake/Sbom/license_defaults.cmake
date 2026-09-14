# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

include(license_asserts.cmake)

set(SBOM_PROJECT_NAME "LicenseDefaults")

_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "${SBOM_PROJECT_NAME}"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "1.0.0"
    INSTALL_SBOM_DIR "sbom_license_defaults"
    DEFAULT_TARGET_LICENSE_CONCLUDED_EXPRESSION "Zlib"
    DEFAULT_TARGET_LICENSE_DECLARED_EXPRESSION "NTP"
)
sbom_test_record_project()

# The project package doesn't record the defaults meant for the target packages.
add_assert_str_exists_in_spdx_v2_3_doc(
    "PackageLicenseConcluded: NOASSERTION\nPackageLicenseDeclared: NOASSERTION\n")

# A target without its own expressions inherits both defaults.
_qt_internal_add_sbom(LicenseDefaultsInherited
    SBOM_ENTITY_TYPE "LIBRARY"
)
assert_package_licenses(LicenseDefaultsInherited CONCLUDED "Zlib" DECLARED "NTP")

# An own concluded expression replaces the default one. The declared default still applies, the
# project asked for it explicitly.
_qt_internal_add_sbom(LicenseDefaultsOwnConcluded
    SBOM_ENTITY_TYPE "LIBRARY"
    LICENSE_CONCLUDED_EXPRESSION "MIT"
)
assert_package_licenses(LicenseDefaultsOwnConcluded CONCLUDED "MIT" DECLARED "NTP")

# An own declared expression replaces the default one.
_qt_internal_add_sbom(LicenseDefaultsOwnDeclared
    SBOM_ENTITY_TYPE "LIBRARY"
    LICENSE_DECLARED_EXPRESSION "ISC"
)
assert_package_licenses(LicenseDefaultsOwnDeclared CONCLUDED "Zlib" DECLARED "ISC")

# A target can opt out of the concluded default alone.
_qt_internal_add_sbom(LicenseDefaultsOptOutConcluded
    SBOM_ENTITY_TYPE "LIBRARY"
    NO_PROJECT_DEFAULT_LICENSE_CONCLUDED_EXPRESSION
)
assert_package_licenses(LicenseDefaultsOptOutConcluded CONCLUDED "NOASSERTION" DECLARED "NTP")

# Or of the declared default alone.
_qt_internal_add_sbom(LicenseDefaultsOptOutDeclared
    SBOM_ENTITY_TYPE "LIBRARY"
    NO_PROJECT_DEFAULT_LICENSE_DECLARED_EXPRESSION
)
assert_package_licenses(LicenseDefaultsOptOutDeclared CONCLUDED "Zlib" DECLARED "NOASSERTION")

# A target can opt out of both defaults.
_qt_internal_add_sbom(LicenseDefaultsOptOut
    SBOM_ENTITY_TYPE "LIBRARY"
    NO_PROJECT_DEFAULT_LICENSE_CONCLUDED_EXPRESSION
    NO_PROJECT_DEFAULT_LICENSE_DECLARED_EXPRESSION
)
assert_package_licenses(LicenseDefaultsOptOut CONCLUDED "NOASSERTION" DECLARED "NOASSERTION")

# After opting out, an own concluded expression is still recorded.
_qt_internal_add_sbom(LicenseDefaultsOptOutOwnConcluded
    SBOM_ENTITY_TYPE "LIBRARY"
    NO_PROJECT_DEFAULT_LICENSE_CONCLUDED_EXPRESSION
    NO_PROJECT_DEFAULT_LICENSE_DECLARED_EXPRESSION
    LICENSE_CONCLUDED_EXPRESSION "Vim"
)
assert_package_licenses(LicenseDefaultsOptOutOwnConcluded CONCLUDED "Vim" DECLARED "NOASSERTION")

# Qt entities don't get the defaults either, their licenses come from the qt entity type handling
# and mirroring is enabled by default.
_qt_internal_add_sbom(LicenseDefaultsQtEntity
    SBOM_ENTITY_TYPE "QT_MODULE"
    __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_LICENSE
    QT_LICENSE_ID "BSD3"
)
assert_package_licenses(LicenseDefaultsQtEntity
    CONCLUDED "BSD-3-Clause" DECLARED "BSD-3-Clause")


# A bundled module qt entity doesn't get the defaults either, but mirroring is disabled by default.
_qt_internal_add_sbom(LicenseDefaultsQtBundledEntity
    SBOM_ENTITY_TYPE "QT_THIRD_PARTY_MODULE"
    __QT_INTERNAL_HANDLE_QT_ENTITY_TYPE_LICENSE
    LICENSE_CONCLUDED_EXPRESSION "curl"
)
assert_package_licenses(LicenseDefaultsQtBundledEntity CONCLUDED "curl" DECLARED "NOASSERTION")

# Third party and system entities don't get the defaults, the project doesn't provide them.
_qt_internal_add_sbom(LicenseDefaultsThirdParty
    SBOM_ENTITY_TYPE "THIRD_PARTY_LIBRARY"
)
assert_package_licenses(LicenseDefaultsThirdParty CONCLUDED "NOASSERTION" DECLARED "NOASSERTION")

# A third party entity that asks for the mirroring gets it, the declared default still doesn't
# apply to it.
_qt_internal_add_sbom(LicenseDefaultsThirdPartyMirror
    SBOM_ENTITY_TYPE "THIRD_PARTY_LIBRARY"
    LICENSE_CONCLUDED_EXPRESSION "IJG"
    MIRROR_LICENSE_DECLARED_EXPRESSION
)
assert_package_licenses(LicenseDefaultsThirdPartyMirror CONCLUDED "IJG" DECLARED "IJG")

_qt_internal_sbom_end_project()

# Separate documents, to check that the defaults can be disabled for a whole project, either
# both at once, or one at a time.
set(LICENSE_DEFAULTS_VARIANT_NAME "LicenseDefaultsAllOff")
set(LICENSE_DEFAULTS_VARIANT_ARGS
    NO_DEFAULT_TARGET_LICENSE_CONCLUDED_EXPRESSION
    NO_DEFAULT_TARGET_LICENSE_DECLARED_EXPRESSION)
set(LICENSE_DEFAULTS_VARIANT_CONCLUDED "NOASSERTION")
set(LICENSE_DEFAULTS_VARIANT_DECLARED "NOASSERTION")
set(LICENSE_DEFAULTS_VARIANT_ABSENT_NEEDLES
    "PackageLicenseConcluded: Zlib"
    "PackageLicenseDeclared: NTP")
add_subdirectory(license_defaults_variants license_defaults_all_off)

set(LICENSE_DEFAULTS_VARIANT_NAME "LicenseDefaultsConcludedOff")
set(LICENSE_DEFAULTS_VARIANT_ARGS NO_DEFAULT_TARGET_LICENSE_CONCLUDED_EXPRESSION)
set(LICENSE_DEFAULTS_VARIANT_CONCLUDED "NOASSERTION")
set(LICENSE_DEFAULTS_VARIANT_DECLARED "NTP")
set(LICENSE_DEFAULTS_VARIANT_ABSENT_NEEDLES "PackageLicenseConcluded: Zlib")
add_subdirectory(license_defaults_variants license_defaults_concluded_off)

set(LICENSE_DEFAULTS_VARIANT_NAME "LicenseDefaultsDeclaredOff")
set(LICENSE_DEFAULTS_VARIANT_ARGS NO_DEFAULT_TARGET_LICENSE_DECLARED_EXPRESSION)
set(LICENSE_DEFAULTS_VARIANT_CONCLUDED "Zlib")
set(LICENSE_DEFAULTS_VARIANT_DECLARED "NOASSERTION")
set(LICENSE_DEFAULTS_VARIANT_ABSENT_NEEDLES "PackageLicenseDeclared: NTP")
add_subdirectory(license_defaults_variants license_defaults_declared_off)

sbom_test_end()

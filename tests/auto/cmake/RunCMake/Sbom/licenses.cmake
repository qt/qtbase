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

# Asserts the concluded license recorded for the file entry whose name contains name_part.
function(assert_file_license name_part concluded)
    if(NOT QT_GENERATE_SBOM OR NOT QT_SBOM_GENERATE_SPDX_V2)
        return()
    endif()

    # Match from the file name through the fixed file fields, so the check can't accidentally match
    # another file's license. The install path is not part of the needle, because it differs
    # between single and multi config builds.
    string(CONCAT needle
        "FileName: [^\n]*${name_part}[^\n]*\n"
        "SPDXID:[^\n]*\n"
        "FileType:[^\n]*\n"
        "FileChecksum:[^\n]*\n"
        "LicenseConcluded: ${concluded}\n"
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
add_assert_str_exists_in_cydx_v1_6_doc("\"expression\": \"MIT\"")

# The concluded option wins over the older option.
_qt_internal_add_sbom(LicenseConcludedOverridesOldOption
    SBOM_ENTITY_TYPE "LIBRARY"
    LICENSE_EXPRESSION "curl"
    LICENSE_CONCLUDED_EXPRESSION "ISC"
)
assert_package_licenses(LicenseConcludedOverridesOldOption CONCLUDED "ISC" DECLARED "NOASSERTION")
add_assert_str_exists_in_cydx_v1_6_doc("\"expression\": \"ISC\"")
add_assert_str_not_exists_in_cydx_v1_6_doc("\"expression\": \"curl\"")

# Both expressions can be set independently.
_qt_internal_add_sbom(LicenseConcludedAndDeclared
    SBOM_ENTITY_TYPE "LIBRARY"
    LICENSE_CONCLUDED_EXPRESSION "BSD-3-Clause"
    LICENSE_DECLARED_EXPRESSION "Unlicense"
)
assert_package_licenses(LicenseConcludedAndDeclared
    CONCLUDED "BSD-3-Clause" DECLARED "Unlicense")
add_assert_str_exists_in_cydx_v1_6_doc("\"expression\": \"BSD-3-Clause\"")
add_assert_str_exists_in_cydx_v1_6_doc("\"value\": \"Unlicense\"")

# A declared expression can be set without a concluded one. In CycloneDX it ends up as the
# component license, with a declared acknowledgement.
_qt_internal_add_sbom(LicenseDeclaredOnly
    SBOM_ENTITY_TYPE "LIBRARY"
    LICENSE_DECLARED_EXPRESSION "NTP"
)
assert_package_licenses(LicenseDeclaredOnly CONCLUDED "NOASSERTION" DECLARED "NTP")
add_assert_str_exists_in_cydx_v1_6_doc("\"expression\": \"NTP\"")
add_assert_str_exists_in_cydx_v1_6_doc("\"acknowledgement\": \"declared\"")
add_assert_str_exists_in_cydx_v1_6_doc("\"acknowledgement\": \"concluded\"")

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

# File level expressions.
# The declared value is a no-op for now, and the compatibility LICENSE_EXPRESSION option
# is ignored due to LICENSE_CONCLUDED_EXPRESSION being set.
set(license_files "custom_files/sources/custom1.txt")
set(license_files_install_path "licfiles")

_qt_internal_add_sbom(LicenseFiles
    SBOM_ENTITY_TYPE "CUSTOM"
)
_qt_internal_sbom_add_files(LicenseFiles
    FILES "${license_files}"
    FILE_TYPE "CUSTOM"
    INSTALL_PATH "${license_files_install_path}"
    INSTALL_PATH_DEBUG "${license_files_install_path}/debug"
    INSTALL_PATH_RELEASE "${license_files_install_path}/release"
    LICENSE_EXPRESSION "curl"
    LICENSE_CONCLUDED_EXPRESSION "X11"
    LICENSE_DECLARED_EXPRESSION "W3C"
)

install(FILES
    ${license_files}
    DESTINATION "${license_files_install_path}"
)

add_assert_str_exists_in_spdx_v2_3_doc(
    "LicenseConcluded: X11\nFileCopyrightText:[^\n]*\nLicenseInfoInFile: NOASSERTION\n")
add_assert_str_not_exists_in_spdx_v2_3_doc("LicenseConcluded: curl")
add_assert_str_not_exists_in_spdx_v2_3_doc("W3C")

# The binary files of a target can get a different expression than the target package.
add_library(lic_binary_files STATIC)
target_sources(lic_binary_files PRIVATE sources/core_helper.cpp)
install(TARGETS lic_binary_files ARCHIVE DESTINATION lib)
_qt_internal_add_sbom(lic_binary_files
    SBOM_ENTITY_TYPE "LIBRARY"
    RUNTIME_PATH bin
    ARCHIVE_PATH lib
    LIBRARY_PATH lib
    LICENSE_CONCLUDED_EXPRESSION "Sleepycat"
    BINARY_FILES_LICENSE_CONCLUDED_EXPRESSION "Vim"
    BINARY_FILES_LICENSE_DECLARED_EXPRESSION "W3C"
)
assert_package_licenses(lic_binary_files CONCLUDED "Sleepycat" DECLARED "NOASSERTION")
assert_file_license("lic_binary_files" "Vim")

# The files of a target can opt out of the package expression entirely.
add_library(lic_no_files_license STATIC)
target_sources(lic_no_files_license PRIVATE sources/core_helper.cpp)
install(TARGETS lic_no_files_license ARCHIVE DESTINATION lib)
_qt_internal_add_sbom(lic_no_files_license
    SBOM_ENTITY_TYPE "LIBRARY"
    RUNTIME_PATH bin
    ARCHIVE_PATH lib
    LIBRARY_PATH lib
    LICENSE_CONCLUDED_EXPRESSION "PostgreSQL"
    NO_TARGET_LICENSE_EXPRESSION_FOR_FILES
)
assert_package_licenses(lic_no_files_license CONCLUDED "PostgreSQL" DECLARED "NOASSERTION")
assert_file_license("lic_no_files_license" "NOASSERTION")
add_assert_str_not_exists_in_spdx_v2_3_doc("\nLicenseConcluded: PostgreSQL")

# Custom file sets inherit the package expression, unless they opt out.
set(license_file_sets_install_path "licfilesets")

_qt_internal_add_sbom(LicenseFileSets
    SBOM_ENTITY_TYPE "CUSTOM"
    LICENSE_CONCLUDED_EXPRESSION "Sleepycat"
)
_qt_internal_sbom_add_files(LicenseFileSets
    FILES "custom_files/sources/res1.txt"
    FILE_TYPE "CUSTOM"
    INSTALL_PATH "${license_file_sets_install_path}"
    INSTALL_PATH_DEBUG "${license_file_sets_install_path}/debug"
    INSTALL_PATH_RELEASE "${license_file_sets_install_path}/release"
)
_qt_internal_sbom_add_files(LicenseFileSets
    FILES "custom_files/sources/res1_source.txt"
    FILE_TYPE "CUSTOM"
    INSTALL_PATH "${license_file_sets_install_path}"
    INSTALL_PATH_DEBUG "${license_file_sets_install_path}/debug"
    INSTALL_PATH_RELEASE "${license_file_sets_install_path}/release"
    NO_TARGET_LICENSE_EXPRESSION
)

install(FILES
    custom_files/sources/res1.txt
    custom_files/sources/res1_source.txt
    DESTINATION "${license_file_sets_install_path}"
)

assert_file_license("res1\\.txt" "Sleepycat")
assert_file_license("res1_source\\.txt" "NOASSERTION")

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
# A declared expression that differs from the concluded one is recorded as a CycloneDX property,
# because CycloneDX v1.6 allows at most one license expression per component.
set(LICENSE_VARIANT_CYDX_NEEDLES
    "\"expression\": \"0BSD\""
    "\"name\": \"spdx:package-license-declared\""
    "\"value\": \"Zlib\"")
add_subdirectory(license_project_variants license_concluded_overrides_compat)

# Only the compatibility option, its value is recorded in both fields.
set(LICENSE_VARIANT_NAME "LicenseCompatOnly")
set(LICENSE_VARIANT_ARGS LICENSE_EXPRESSION "MIT-0")
unset(LICENSE_VARIANT_ABSENT_NEEDLES)
set(LICENSE_VARIANT_CONCLUDED "MIT-0")
set(LICENSE_VARIANT_DECLARED "MIT-0")
# The same value in both fields needs no extra property.
set(LICENSE_VARIANT_CYDX_NEEDLES
    "\"expression\": \"MIT-0\""
    "\"acknowledgement\": \"concluded\"")
add_subdirectory(license_project_variants license_compat_only)

# Only the new concluded option, the declared field stays empty.
set(LICENSE_VARIANT_NAME "LicenseConcludedOnly")
set(LICENSE_VARIANT_ARGS PROJECT_LICENSE_CONCLUDED_EXPRESSION "Vim")
set(LICENSE_VARIANT_CONCLUDED "Vim")
set(LICENSE_VARIANT_DECLARED "NOASSERTION")
set(LICENSE_VARIANT_CYDX_NEEDLES "\"expression\": \"Vim\"")
add_subdirectory(license_project_variants license_concluded_only)

# Only the new declared option, the concluded field stays empty.
set(LICENSE_VARIANT_NAME "LicenseDeclaredOnlyProject")
set(LICENSE_VARIANT_ARGS PROJECT_LICENSE_DECLARED_EXPRESSION "W3C")
set(LICENSE_VARIANT_CONCLUDED "NOASSERTION")
set(LICENSE_VARIANT_DECLARED "W3C")
set(LICENSE_VARIANT_CYDX_NEEDLES
    "\"expression\": \"W3C\""
    "\"acknowledgement\": \"declared\"")
add_subdirectory(license_project_variants license_declared_only)

# No license options at all.
set(LICENSE_VARIANT_NAME "LicenseNoneAtAll")
set(LICENSE_VARIANT_ARGS "")
set(LICENSE_VARIANT_CONCLUDED "NOASSERTION")
set(LICENSE_VARIANT_DECLARED "NOASSERTION")
unset(LICENSE_VARIANT_CYDX_NEEDLES)
add_subdirectory(license_project_variants license_none_at_all)

sbom_test_end()

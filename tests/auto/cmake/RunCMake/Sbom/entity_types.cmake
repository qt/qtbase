# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

set(SBOM_PROJECT_NAME "EntityTypes")
_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "${SBOM_PROJECT_NAME}"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "1.0.0"
)
sbom_test_record_project()

function(check_sbom_entity_type target)
    set(opt_args "")
    set(single_args
        TYPE
        EXPECTED_INFIX
        EXPECTED_PURPOSE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")

    _qt_internal_add_sbom(${target}
        TYPE "${arg_TYPE}"
    )

    if(NOT QT_GENERATE_SBOM OR NOT QT_SBOM_GENERATE_SPDX_V2)
        return()
    endif()

    _qt_internal_sbom_get_spdx_id_for_target(${target} spdx_id)

    if(NOT spdx_id MATCHES "-${arg_EXPECTED_INFIX}-")
        message(FATAL_ERROR
            "Expected SPDX id for package '${target}' (type ${arg_TYPE}) to contain infix "
            "'-${arg_EXPECTED_INFIX}-', but got '${spdx_id}'.")
    endif()

    # Match from the SPDXID line through the fixed package fields down to PrimaryPackagePurpose,
    # so the check can't accidentally match another package's purpose.
    string(CONCAT needle
        "SPDXID: ${spdx_id}\n"
        "PackageDownloadLocation:[^\n]*\n"
        "PackageVersion:[^\n]*\n"
        "PackageSupplier:[^\n]*\n"
        "PackageLicenseConcluded:[^\n]*\n"
        "PackageLicenseDeclared:[^\n]*\n"
        "PackageCopyrightText:[^\n]*\n"
        "PrimaryPackagePurpose: ${arg_EXPECTED_PURPOSE}"
    )
    add_assert_str_exists_in_spdx_v2_3_doc("${needle}")
endfunction()

check_sbom_entity_type(EntitySources
    TYPE SOURCES_ENTITY_TYPE
    EXPECTED_INFIX "sources"
    EXPECTED_PURPOSE "LIBRARY"
)
check_sbom_entity_type(EntityObjectLibrary
    TYPE OBJECT_LIBRARY_ENTITY_TYPE
    EXPECTED_INFIX "object-library"
    EXPECTED_PURPOSE "LIBRARY"
)
check_sbom_entity_type(EntityFramework
    TYPE FRAMEWORK_ENTITY_TYPE
    EXPECTED_INFIX "framework"
    EXPECTED_PURPOSE "FRAMEWORK"
)
check_sbom_entity_type(EntityFiles
    TYPE FILES_ENTITY_TYPE
    EXPECTED_INFIX "files"
    EXPECTED_PURPOSE "FILE"
)
check_sbom_entity_type(EntityArchives
    TYPE ARCHIVES
    EXPECTED_INFIX "archives"
    EXPECTED_PURPOSE "ARCHIVE"
)
check_sbom_entity_type(EntityInstallers
    TYPE INSTALLERS
    EXPECTED_INFIX "installers"
    EXPECTED_PURPOSE "INSTALL"
)

_qt_internal_sbom_end_project()

sbom_test_end()

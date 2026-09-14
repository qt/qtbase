# Assertion helpers shared by the license test cases.

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

function(check_exists file)
    if(NOT EXISTS "${file}")
        get_filename_component(file_dir "${file}" DIRECTORY)
        file(GLOB dir_contents "${file_dir}/*")
        string(APPEND RunCMake_TEST_FAILED "${file} does not exist\n. "
            "Contents of directory ${file_dir}:\n ${dir_contents}\n")
    endif()
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

function(check_not_exists file)
    if(EXISTS "${file}")
        get_filename_component(file_dir "${file}" DIRECTORY)
        file(GLOB dir_contents "${file_dir}/*")
        string(APPEND RunCMake_TEST_FAILED "${file} exists\n. "
            "Contents of directory ${file_dir}:\n ${dir_contents}\n")
    endif()
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

# Asserts needle exists in the document at path doc_path, or that it does not exist if ABSENT is
# passed. format_name is the document format shown in the failure message.
function(sbom_assert_needle_in_document doc_path needle format_name)
    set(opt_args
        ABSENT
    )
    set(single_args "")
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 3 arg "${opt_args}" "${single_args}" "${multi_args}")

    if(arg_ABSENT)
        set(check_kind "needle absence")
    else()
        set(check_kind "needle")
    endif()

    if(NOT EXISTS "${doc_path}")
        string(APPEND RunCMake_TEST_FAILED
            "Cannot check ${check_kind}; ${format_name} document '${doc_path}' does not exist.\n")
        set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
        return()
    endif()

    file(READ "${doc_path}" doc_contents)
    if(arg_ABSENT AND doc_contents MATCHES "${needle}")
        string(APPEND RunCMake_TEST_FAILED
            "Expected NOT to find '${needle}' in ${format_name} document '${doc_path}', "
            "but it was found.\n")
    elseif(NOT arg_ABSENT AND NOT doc_contents MATCHES "${needle}")
        string(APPEND RunCMake_TEST_FAILED
            "Expected to find '${needle}' in ${format_name} document '${doc_path}', "
            "but it was not found.\n")
    endif()
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

# Asserts needle exists in the SPDX v2.3 document at path doc_path.
function(sbom_assert_needle_in_spdx doc_path needle)
    sbom_assert_needle_in_document("${doc_path}" "${needle}" "SPDX v2.3")
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

# Asserts needle does NOT exist in the SPDX v2.3 document at path doc_path.
function(sbom_assert_needle_not_in_spdx doc_path needle)
    sbom_assert_needle_in_document("${doc_path}" "${needle}" "SPDX v2.3" ABSENT)
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

# Asserts needle exists in the CycloneDX v1.6 document at path doc_path.
function(sbom_assert_needle_in_cydx doc_path needle)
    sbom_assert_needle_in_document("${doc_path}" "${needle}" "CycloneDX v1.6")
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

# Asserts needle does NOT exist in the CycloneDX v1.6 document at path doc_path.
function(sbom_assert_needle_not_in_cydx doc_path needle)
    sbom_assert_needle_in_document("${doc_path}" "${needle}" "CycloneDX v1.6" ABSENT)
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

# Verifies the recorded CycloneDX dependencies of one document's known targets.
function(sbom_check_cydx_deps_for_document doc_path doc_id)
    if(NOT doc_path OR NOT EXISTS "${doc_path}")
        return()
    endif()

    # Check that the targets category var is defined in the parent scope, even if it's empty.
    set(known_targets_var "SBOM_DOC_${doc_id}_KNOWN_TARGETS")
    if(NOT DEFINED "${known_targets_var}")
        string(APPEND RunCMake_TEST_FAILED
            "Expected variable '${known_targets_var}' to be defined in 'result.cmake' file.\n")
        set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
        return()
    endif()

    set(known_targets "${${known_targets_var}}")
    if(NOT known_targets)
        return()
    endif()

    file(READ "${doc_path}" cydx_contents)
    string(JSON dependencies GET "${cydx_contents}" "dependencies")
    string(JSON dependencies_length LENGTH "${dependencies}")

    # Collect component spdx ids from the dependencies key of the cdx document.
    set(doc_spdx_ids "")

    set(idx 0)
    while(idx LESS dependencies_length)
        string(JSON dependency GET "${dependencies}" "${idx}")

        string(JSON ref GET "${dependency}" "ref")
        list(APPEND doc_spdx_ids "${ref}")

        string(JSON depends_on_length ERROR_VARIABLE depends_on_error LENGTH "${dependency}"
            "dependsOn")
        if(depends_on_error STREQUAL "NOTFOUND")
            set(dep_idx 0)
            while(dep_idx LESS depends_on_length)
                string(JSON depends_on_item GET "${dependency}" "dependsOn" "${dep_idx}")
                math(EXPR dep_idx "${dep_idx} + 1")

                # Collect the deps of each component spdx id.
                list(APPEND doc_spdx_ids_${ref}_deps "${depends_on_item}")
            endwhile()
        endif()

        math(EXPR idx "${idx} + 1")
    endwhile()

    # Sort deps, so that equality checks of lists work.
    foreach(spdx_id IN LISTS doc_spdx_ids)
        list(SORT doc_spdx_ids_${spdx_id}_deps)
    endforeach()

    # Go through the recorded targets, and confirm that the recorded spdx ids and dependencies
    # match those in the CycloneDX document.
    foreach(target IN LISTS known_targets)
        set(target_spdx_id "${SBOM_DOC_${doc_id}_TARGET_${target}_SPDX_ID}")
        if(NOT "${target_spdx_id}" IN_LIST doc_spdx_ids)
            string(APPEND RunCMake_TEST_FAILED
                "Component ID '${target_spdx_id}' (target ${target}) not found in CycloneDX "
                "document '${doc_path}'.\n")
        endif()

        set(expected_deps "${SBOM_DOC_${doc_id}_TARGET_${target}_DEPS}")
        set(found_deps "${doc_spdx_ids_${target_spdx_id}_deps}")
        if(NOT expected_deps STREQUAL found_deps)
            string(APPEND RunCMake_TEST_FAILED
                "Component ID '${target_spdx_id}' (target ${target}) dependencies do not match.\n"
                "  Expected: '${expected_deps}'\n  Found:    '${found_deps}'\n")
        endif()
    endforeach()
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

macro(include_root_result_file_and_run_checks)
    # Check that the correct option values are used for the project root sbom.
    set(root_result_file "${RunCMake_TEST_BINARY_DIR}/result.cmake")
    if(EXISTS "${root_result_file}")
        include("${root_result_file}")
        assert_expected_sbom_option_values()
        sbom_check_documents()
        sbom_check_none_case_absence()
    endif()
endmacro()

# Checks that the various QT_GENERATE_SBOM options have expected values depending on how the
# project was configured.
macro(assert_expected_sbom_option_values)
    if(NOT EXPECTED_QT_GENERATE_SBOM STREQUAL "" AND
            NOT "${EXPECTED_QT_GENERATE_SBOM}" STREQUAL "${RESULT_QT_GENERATE_SBOM}")
        string(APPEND RunCMake_TEST_FAILED
            "QT_GENERATE_SBOM is ${RESULT_QT_GENERATE_SBOM}, "
            "expected ${EXPECTED_QT_GENERATE_SBOM} \n")
    endif()

    if(NOT EXPECTED_QT_SBOM_GENERATE_SPDX_V2 STREQUAL "" AND
            NOT "${EXPECTED_QT_SBOM_GENERATE_SPDX_V2}" STREQUAL
            "${RESULT_QT_SBOM_GENERATE_SPDX_V2}")
        string(APPEND RunCMake_TEST_FAILED
            "QT_SBOM_GENERATE_SPDX_V2 is ${RESULT_QT_SBOM_GENERATE_SPDX_V2}, "
            "expected ${EXPECTED_QT_SBOM_GENERATE_SPDX_V2} \n")
    endif()

    if(NOT EXPECTED_QT_SBOM_GENERATE_CYDX_V1_6 STREQUAL "" AND
            NOT "${EXPECTED_QT_SBOM_GENERATE_CYDX_V1_6}" STREQUAL
            "${RESULT_QT_SBOM_GENERATE_CYDX_V1_6}")
        string(APPEND RunCMake_TEST_FAILED
            "QT_SBOM_GENERATE_CYDX_V1_6 is ${RESULT_QT_SBOM_GENERATE_CYDX_V1_6}, "
            "expected ${EXPECTED_QT_SBOM_GENERATE_CYDX_V1_6} \n")
    endif()
endmacro()

# Runs all per-document checks.
function(sbom_check_documents)
    foreach(doc_id IN LISTS SBOM_DOCUMENT_IDS)
        foreach(f IN LISTS SBOM_DOC_${doc_id}_PRESENT)
            check_exists("${f}")
        endforeach()

        foreach(f IN LISTS SBOM_DOC_${doc_id}_ABSENT)
            check_not_exists("${f}")
        endforeach()

        foreach(needle IN LISTS SBOM_DOC_${doc_id}_SPDX_NEEDLES)
            sbom_assert_needle_in_spdx("${SBOM_DOC_${doc_id}_SPDX}" "${needle}")
        endforeach()

        foreach(needle IN LISTS SBOM_DOC_${doc_id}_SPDX_ABSENT_NEEDLES)
            sbom_assert_needle_not_in_spdx("${SBOM_DOC_${doc_id}_SPDX}" "${needle}")
        endforeach()

        foreach(needle IN LISTS SBOM_DOC_${doc_id}_CYDX_NEEDLES)
            sbom_assert_needle_in_cydx("${SBOM_DOC_${doc_id}_CYDX}" "${needle}")
        endforeach()

        foreach(needle IN LISTS SBOM_DOC_${doc_id}_CYDX_ABSENT_NEEDLES)
            sbom_assert_needle_not_in_cydx("${SBOM_DOC_${doc_id}_CYDX}" "${needle}")
        endforeach()

        sbom_check_cydx_deps_for_document("${SBOM_DOC_${doc_id}_CYDX}" "${doc_id}")
    endforeach()

    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

# Check that no documents exist in the install tree when SBOM generation is disabled.
# sbom_test_record_project cannot provide document paths in that case (begin_project returns early)
# so we just glob and assert there are no sbom docs at all.
function(sbom_check_none_case_absence)
    if(NOT RESULT_QT_GENERATE_SBOM)
        file(GLOB_RECURSE stray_docs
            "${RunCMake_TEST_BINARY_DIR}/installed/*.spdx"
            "${RunCMake_TEST_BINARY_DIR}/installed/*.spdx.json"
            "${RunCMake_TEST_BINARY_DIR}/installed/*.cdx.json")
        if(stray_docs)
            string(APPEND RunCMake_TEST_FAILED
                "Expected no SBOM documents when QT_GENERATE_SBOM is OFF, but found: "
                "${stray_docs}\n")
        endif()
    endif()

    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

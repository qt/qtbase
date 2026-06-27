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

# Asserts that the given needle string exists in the SPDX v2.3 document.
function(assert_str_exists_in_spdx_v2_3_doc needle)
    if(NOT SPDX_V2_3_DOCUMENTS)
        return()
    endif()
    list(GET SPDX_V2_3_DOCUMENTS 0 doc_name)
    file(READ "${doc_name}" doc_contents)
    if(NOT doc_contents MATCHES "${needle}")
        string(APPEND RunCMake_TEST_FAILED
            "Expected to find '${needle}' in SPDX v2.3 document '${doc_name}',"
            " but it was not found.\n\n doc contents \n${doc_contents}}.\n")
    endif()
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

# Asserts that all needles in SPDX_V2_3_STR_ASSERT_NEEDLES exist in the SPDX v2.3 document.
function(check_assert_str_exists_in_spdx_v2_3_doc_needles)
    foreach(needle IN LISTS SPDX_V2_3_STR_ASSERT_NEEDLES)
        assert_str_exists_in_spdx_v2_3_doc("${needle}")
    endforeach()
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

# Asserts that the given needle string exists in the CycloneDX v1.6 document.
# Currently unused.
function(assert_str_exists_in_cydx_v1_6_doc needle)
    if(NOT CYDX_V1_6_DOCUMENTS)
        return()
    endif()

    list(GET CYDX_V1_6_DOCUMENTS 0 doc_name)
    file(READ "${doc_name}" doc_contents)
    if(NOT doc_contents MATCHES "${needle}")
        string(APPEND RunCMake_TEST_FAILED
            "Expected to find '${needle}' in CyDX v1.6 document '${doc_name}',"
            " but it was not found.\n")
    endif()
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

# Asserts that all needles in CYDX_V1_6_STR_ASSERT_NEEDLES exist in the CycloneDX v1.6 document.
# Currently unused.
function(check_assert_str_exists_in_cydx_v1_6_doc_needles)
    foreach(needle IN LISTS CYDX_V1_6_STR_ASSERT_NEEDLES)
        assert_str_exists_in_cydx_v1_6_doc("${needle}")
    endforeach()
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

# Gets the first CycloneDX v1.6 document path from the parent scope.
function(get_cydx_v1_6_doc_path out_var)
    if(NOT CYDX_V1_6_DOCUMENTS)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    list(GET CYDX_V1_6_DOCUMENTS 0 doc_path)
    set(${out_var} "${doc_path}" PARENT_SCOPE)
endfunction()

# Gets the contents of the first CycloneDX v1.6 document from the parent scope.
function(get_cydx_v1_6_doc_contents out_var)
    if(NOT CYDX_V1_6_DOCUMENTS)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    list(GET CYDX_V1_6_DOCUMENTS 0 doc_name)

    if(NOT EXISTS "${doc_name}")
        string(APPEND RunCMake_TEST_FAILED "CycloneDX document '${doc_name}' does not exist.\n")
        set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    file(READ "${doc_name}" doc_contents)
    set(${out_var} "${doc_contents}" PARENT_SCOPE)
endfunction()

# Verifies that the given targets have their dependencies correctly represented in
# the CycloneDX v1.6 document.
# The expected dependencies for each target must be set in the parent scope using
# add_extra_code_to_result_file.
function(check_cydx_v1_6_dependencies)
    set(opt_args "")
    set(single_args
        TARGETS_CATEGORY
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")

    # Var is set in parent scope.
    if(NOT CYDX_V1_6_DOCUMENTS)
        return()
    endif()

    if(NOT arg_TARGETS_CATEGORY)
        string(APPEND RunCMake_TEST_FAILED
            "Expected argument 'TARGETS_CATEGORY' to be passed.\n")
        set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
        return()
    endif()

    # Taken from parent scope.
    get_cydx_v1_6_doc_contents(cydx_contents)
    if(NOT cydx_contents)
        string(APPEND RunCMake_TEST_FAILED
            "Failed to get contents of CycloneDX document for dependency checks.\n")
        set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
        return()
    endif()
    get_cydx_v1_6_doc_path(cydx_path)

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

    # Check that the targets category var is defined in the parent scope, even if it's empty.
    set(targets_var "KNOWN_TARGETS_${arg_TARGETS_CATEGORY}")
    if(NOT DEFINED "${targets_var}")
        string(APPEND RunCMake_TEST_FAILED
            "Expected variable '${targets_var}' to be defined in 'result.cmake' file.\n")
        set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
        return()
    endif()

    # Go through the recorded targets, and confirm that the spdx ids and dependencies specified
    # in the parent scope match those in the CycloneDX document.
    set(targets "${${targets_var}}")
    foreach(target IN LISTS targets)
        # This var is set in the parent scope.
        set(target_spdx_id "${${target}_SPDX_ID}")

        if(NOT "${target_spdx_id}" IN_LIST doc_spdx_ids)
            string(APPEND RunCMake_TEST_FAILED
                "Component ID '${target_spdx_id}' not found in CycloneDX document"
                " '${cydx_path}'.\n")
        endif()
        # This var is set in parent scope.
        set(expected_target_deps "${${target}_DEPS}")

        set(current_docs_target_deps "${doc_spdx_ids_${target_spdx_id}_deps}")
        if(NOT expected_target_deps STREQUAL current_docs_target_deps)
            string(APPEND RunCMake_TEST_FAILED
                "Component ID '${target_spdx_id}' dependencies do not match expected "
                "dependencies.\n"
                "  Expected: '${expected_target_deps}'\n"
                "  Found:    '${current_docs_target_deps}'\n")
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
    endif()
endmacro()

macro(assert_expected_sbom_option_values)
    if(NOT EXPECTED_QT_GENERATE_SBOM STREQUAL "" AND
            NOT "${EXPECTED_QT_GENERATE_SBOM}" STREQUAL "${RESULT_QT_GENERATE_SBOM}")
        string(APPEND RunCMake_TEST_FAILED
            "QT_GENERATE_SBOM is ${RESULT_QT_GENERATE_SBOM}, expected ${EXPECTED_QT_GENERATE_SBOM} \n")
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

macro(include_all_result_files_and_run_checks)
    # Glob for all result.cmake files recursively in the root of the test binary dir, and run checks
    # for each of them.
    file(GLOB_RECURSE result_files
        "${RunCMake_TEST_BINARY_DIR}/**/result.cmake"
    )

    # Confirm that the all subproject sbom files are installed, including the root one.
    foreach(result_file IN LISTS result_files)
        include("${result_file}")
        assert_sbom_doc_existence()
    endforeach()

    check_assert_str_exists_in_spdx_v2_3_doc_needles()
    check_assert_str_exists_in_cydx_v1_6_doc_needles()
endmacro()

macro(assert_sbom_doc_existence)
    foreach(sbom_doc IN LISTS SBOM_DOCUMENTS)
        check_exists("${sbom_doc}")
    endforeach()

    foreach(sbom_doc IN LISTS NO_SBOM_DOCUMENTS)
        check_not_exists("${sbom_doc}")
    endforeach()
endmacro()

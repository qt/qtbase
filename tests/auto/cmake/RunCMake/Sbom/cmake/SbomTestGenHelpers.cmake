# Adds a string that is expected to exist in the SPDX v2.3 document.
# This gets written to the result.cmake file, which is then checked by the check.cmake file.
function(add_assert_str_exists_in_spdx_v2_3_doc needle)
    list(APPEND SPDX_V2_3_STR_ASSERT_NEEDLES "${needle}")
    set(SPDX_V2_3_STR_ASSERT_NEEDLES "${SPDX_V2_3_STR_ASSERT_NEEDLES}" PARENT_SCOPE)
endfunction()

# Adds extra code to the result.cmake file that will be generated.
function(add_extra_code_to_result_file contents)
    set(new_contents "${EXTRA_RESULT_CODE}
${contents}
")
    set(EXTRA_RESULT_CODE "${new_contents}" PARENT_SCOPE)
endfunction()

# Adds the given target to the list of known targets in the result.cmake file, along with
# metadata like category and SPDX ID. Used by the check.cmake infrastructure to do checks.
function(add_known_target target)
    set(opt_args "")
    set(single_args
        CATEGORY
        SPDX_ID
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")

    add_extra_code_to_result_file("
list(APPEND KNOWN_TARGETS \"${target}\")
")

    if(arg_CATEGORY)
        add_extra_code_to_result_file("
list(APPEND KNOWN_TARGETS_${arg_CATEGORY} \"${target}\")
")
    endif()

    if(arg_SPDX_ID)
        add_extra_code_to_result_file("
set(${target}_SPDX_ID \"${arg_SPDX_ID}\")
")
    endif()

    bubble_up_extra_result_code()
endfunction()

# Adds the expected dependencies for the given target to the extra result code.
function(add_cydx_v1_6_deps_to_result_file target)
    set(opt_args "")
    set(single_args "")
    set(multi_args
        DEPS
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")

    add_extra_code_to_result_file("
set(${target}_DEPS \"${arg_DEPS}\")
")
    bubble_up_extra_result_code()
endfunction()

macro(bubble_up_extra_result_code)
    set(EXTRA_RESULT_CODE "${EXTRA_RESULT_CODE}" PARENT_SCOPE)
endmacro()

macro(sbom_test_begin)
    # Currently a no-op.
endmacro()

macro(sbom_test_end)
    if(NOT SBOM_PROJECT_NAME)
        set(SBOM_PROJECT_NAME "${PROJECT_NAME}")
    endif()
    # Convert to lower case, otherwise on case-sensitive filesystems the generated
    # filenames may not match the expected ones.
    string(TOLOWER "${SBOM_PROJECT_NAME}" SBOM_PROJECT_NAME)

    if(NOT SBOM_VERSION)
        set(SBOM_VERSION "1.0.0")
    endif()

    if(NOT SBOM_INSTALL_DIR)
        set(SBOM_INSTALL_DIR "sbom")
    endif()

    set(sbom_document_base_name "${SBOM_PROJECT_NAME}-${SBOM_VERSION}")
    set(sbom_install_dir "${CMAKE_BINARY_DIR}/installed/${SBOM_INSTALL_DIR}")

    set(spdx_file "${sbom_install_dir}/${sbom_document_base_name}.spdx")
    set(spdx_json_file "${sbom_install_dir}/${sbom_document_base_name}.spdx.json")
    set(cydx_file "${sbom_install_dir}/${sbom_document_base_name}.cdx.json")

    set(sbom_documents "")
    set(no_sbom_documents "")
    set(spdx_v2_3_documents "")
    set(cydx_v1_6_documents "")

    if(FORMAT_CASE STREQUAL "spdx23" OR FORMAT_CASE STREQUAL "all")
        if(QT_SBOM_GENERATE_SPDX_V2)
            list(APPEND sbom_documents "${spdx_file}")
            list(APPEND spdx_v2_3_documents "${spdx_file}")
        else()
            list(APPEND no_sbom_documents "${spdx_file}")
        endif()

        if(QT_SBOM_GENERATE_SPDX_V2_JSON)
            list(APPEND sbom_documents "${spdx_json_file}")
        else()
            list(APPEND no_sbom_documents "${spdx_json_file}")
        endif()
    endif()

    if(FORMAT_CASE STREQUAL "cydx16" OR FORMAT_CASE STREQUAL "all")
        if(QT_SBOM_GENERATE_CYDX_V1_6)
            list(APPEND sbom_documents "${cydx_file}")
            list(APPEND cydx_v1_6_documents "${cydx_file}")
        else()
            list(APPEND no_sbom_documents "${cydx_file}")
        endif()
    endif()

    if(FORMAT_CASE STREQUAL "none")
        set(no_sbom_documents ${spdx_file} ${spdx_json_file} ${cydx_file})
        set(sbom_documents "")
    endif()

    # These values will be used by the check.cmake script after installation.
    file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/result.cmake"
        CONTENT
            "
    set(SBOM_DOCUMENTS \"${sbom_documents}\")
    set(NO_SBOM_DOCUMENTS \"${no_sbom_documents}\")
    set(SPDX_V2_3_DOCUMENTS \"${spdx_v2_3_documents}\")
    set(CYDX_V1_6_DOCUMENTS \"${cydx_v1_6_documents}\")
    set(EXPECTED_QT_GENERATE_SBOM \"${EXPECTED_QT_GENERATE_SBOM}\")
    set(EXPECTED_QT_SBOM_GENERATE_SPDX_V2 \"${EXPECTED_QT_SBOM_GENERATE_SPDX_V2}\")
    set(EXPECTED_QT_SBOM_GENERATE_CYDX_V1_6 \"${EXPECTED_QT_SBOM_GENERATE_CYDX_V1_6}\")
    set(RESULT_QT_GENERATE_SBOM \"${QT_GENERATE_SBOM}\")
    set(RESULT_QT_SBOM_GENERATE_SPDX_V2 \"${QT_SBOM_GENERATE_SPDX_V2}\")
    set(RESULT_QT_SBOM_GENERATE_CYDX_V1_6 \"${QT_SBOM_GENERATE_CYDX_V1_6}\")
    set(SPDX_V2_3_STR_ASSERT_NEEDLES \"${SPDX_V2_3_STR_ASSERT_NEEDLES}\")
    set(CYDX_V1_6_STR_ASSERT_NEEDLES \"${CYDX_V1_6_STR_ASSERT_NEEDLES}\")
    ${EXTRA_RESULT_CODE}
    "
    )
endmacro()

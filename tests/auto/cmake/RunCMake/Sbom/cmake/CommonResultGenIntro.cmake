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

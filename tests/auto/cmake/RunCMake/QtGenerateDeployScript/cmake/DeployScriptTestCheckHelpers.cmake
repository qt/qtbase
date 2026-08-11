# Returns the generated deploy scripts in the test binary dir.
# NAME_PATTERN should be a glob to match the file names
#  if not set, defaults to the deploy app script pattern.
function(deploy_script_get_generated out_var)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "NAME_PATTERN" "")
    if(NOT arg_NAME_PATTERN)
        set(arg_NAME_PATTERN "deploy_app_*")
    endif()
    file(GLOB deploy_scripts "${RunCMake_TEST_BINARY_DIR}/.qt/${arg_NAME_PATTERN}.cmake")
    set(${out_var} "${deploy_scripts}" PARENT_SCOPE)
endfunction()

# Common part of the assertions below.
# Returns the matching scripts, or fails the test and returns an empty list if there are none.
function(_deploy_script_require_generated out_var name_pattern)
    deploy_script_get_generated(deploy_scripts NAME_PATTERN "${name_pattern}")
    if(NOT deploy_scripts)
        string(APPEND RunCMake_TEST_FAILED
            "No generated deploy script matching '${name_pattern}' found in "
            "${RunCMake_TEST_BINARY_DIR}/.qt\n")
        set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
    endif()
    set(${out_var} "${deploy_scripts}" PARENT_SCOPE)
endfunction()

# Asserts that at least one generated deploy script matches NAME_PATTERN and that each of
# them contains needle. description says what was expected for the failure message.
function(deploy_script_assert_contains needle description)
    cmake_parse_arguments(PARSE_ARGV 2 arg "" "NAME_PATTERN" "")
    _deploy_script_require_generated(deploy_scripts "${arg_NAME_PATTERN}")
    foreach(deploy_script IN LISTS deploy_scripts)
        file(READ "${deploy_script}" content)
        string(FIND "${content}" "${needle}" pos)
        if(pos EQUAL -1)
            string(APPEND RunCMake_TEST_FAILED
                "${deploy_script} does not contain ${description}.\n"
                "Expected to find:\n${needle}\n"
                "Actual content:\n${content}\n")
        endif()
    endforeach()
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

# The opposite of deploy_script_assert_contains(), for values that have to be absent.
function(deploy_script_assert_does_not_contain needle description)
    cmake_parse_arguments(PARSE_ARGV 2 arg "" "NAME_PATTERN" "")
    _deploy_script_require_generated(deploy_scripts "${arg_NAME_PATTERN}")
    foreach(deploy_script IN LISTS deploy_scripts)
        file(READ "${deploy_script}" content)
        string(FIND "${content}" "${needle}" pos)
        if(NOT pos EQUAL -1)
            string(APPEND RunCMake_TEST_FAILED
                "${deploy_script} contains ${description}, but should not.\n"
                "Expected not to find:\n${needle}\n"
                "Actual content:\n${content}\n")
        endif()
    endforeach()
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

# Asserts that no deploy script matching NAME_PATTERN was generated at all.
function(deploy_script_assert_none_generated)
    cmake_parse_arguments(PARSE_ARGV 0 arg "" "NAME_PATTERN" "")
    deploy_script_get_generated(deploy_scripts NAME_PATTERN "${arg_NAME_PATTERN}")
    if(deploy_scripts)
        string(APPEND RunCMake_TEST_FAILED
            "Expected no generated deploy script, but found: ${deploy_scripts}\n")
    endif()
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

# Asserts how often regex appears in the output of the configure run. Used to check that a
# warning is shown once per cmake run rather than once per call.
function(deploy_script_assert_output_occurrences regex expected_count description)
    string(REGEX MATCHALL "${regex}" matches "${actual_stdout}")
    list(LENGTH matches count)
    if(NOT count EQUAL expected_count)
        string(APPEND RunCMake_TEST_FAILED
            "Expected ${description} ${expected_count} time(s), got ${count}.\n"
            "Actual output:\n${actual_stdout}\n")
    endif()
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

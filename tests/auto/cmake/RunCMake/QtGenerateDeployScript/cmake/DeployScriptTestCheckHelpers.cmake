# Returns the app deploy scripts that the test project generated.
function(deploy_script_get_generated out_var)
    file(GLOB deploy_scripts "${RunCMake_TEST_BINARY_DIR}/.qt/deploy_app_*.cmake")
    set(${out_var} "${deploy_scripts}" PARENT_SCOPE)
endfunction()

# Asserts that at least one app deploy script was generated and that each of them contains
# needle. description says what was expected, for the failure message.
function(deploy_script_assert_contains needle description)
    deploy_script_get_generated(deploy_scripts)
    if(NOT deploy_scripts)
        string(APPEND RunCMake_TEST_FAILED
            "No generated deploy script found in ${RunCMake_TEST_BINARY_DIR}/.qt\n")
        set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
        return()
    endif()

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

# Asserts that no app deploy script was generated at all.
function(deploy_script_assert_none_generated)
    deploy_script_get_generated(deploy_scripts)
    if(deploy_scripts)
        string(APPEND RunCMake_TEST_FAILED
            "Expected no generated deploy script, but found: ${deploy_scripts}\n")
    endif()
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

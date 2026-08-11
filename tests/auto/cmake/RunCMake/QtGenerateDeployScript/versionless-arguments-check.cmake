include("${CMAKE_CURRENT_LIST_DIR}/cmake/DeployScriptTestCheckHelpers.cmake")

# The CONTENT value has to reach the generated script unchanged, including the ${} reference,
# which only gets expanded when the deploy script runs.
set(expected_content [==[
# backslash: a\b
# regex: foo\.dylib
# quote: say "hi"
# dollar: ${QT_DEPLOY_PREFIX}
]==])

deploy_script_assert_contains("${expected_content}" "the CONTENT value unchanged"
    NAME_PATTERN "deploy_fidelity*")

# Both the single and the doubled backslashes have to be in the app deploy script as-is.
deploy_script_assert_contains([==[-backslash=a\b]==] "the single backslash value")
deploy_script_assert_contains([==[-doubled=a\\b]==] "the doubled backslash value")

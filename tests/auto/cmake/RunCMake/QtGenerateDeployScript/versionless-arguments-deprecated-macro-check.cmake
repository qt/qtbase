include("${CMAKE_CURRENT_LIST_DIR}/cmake/DeployScriptTestCheckHelpers.cmake")

# The project asked for OLD explicitly, so there is nothing to warn about.
deploy_script_assert_output_occurrences("QTP0008 is not set" 0
    "a warning about the policy")

# The OLD behavior consumed one level of escaping, so only a single backslash is left.
deploy_script_assert_contains([==[-doubled=a\b]==] "the value with one backslash left")
deploy_script_assert_does_not_contain([==[-doubled=a\\b]==]
    "two backslashes, so the macro did not consume the extra level of escaping")

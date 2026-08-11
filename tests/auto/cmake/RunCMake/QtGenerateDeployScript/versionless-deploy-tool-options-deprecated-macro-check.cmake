include("${CMAKE_CURRENT_LIST_DIR}/cmake/DeployScriptTestCheckHelpers.cmake")

# Assert that the old forwarding was really used, so that the case can't pass just because
# the command failed for some other reason. The value has a backslash the old forwarding
# cannot represent, so the pre-flight warning points at the policy before the call fails.
deploy_script_assert_output_occurrences("can't represent" 1
    "the warning about a value the old forwarding cannot represent")

# The values are mangled while the macro forwards them, so the command fails before it gets
# to generate anything.
deploy_script_assert_none_generated()

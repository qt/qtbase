include("${CMAKE_CURRENT_LIST_DIR}/cmake/DeployScriptTestCheckHelpers.cmake")

# The values are mangled while the macro forwards them, so the command fails before it gets
# to generate anything. Assert that no deploy script is created.
deploy_script_assert_none_generated()

include(QtRunCMake)

set(cmake_opts "-DQt6_DIR=${Qt6_DIR}")

# Check that configuration and build suceeds when a project does not set an explicit policy value.
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/project-policy-none-build")
run_cmake_with_options(project-policy-none ${cmake_opts})

set(RunCMake_TEST_NO_CLEAN TRUE)
run_cmake_command(project-policy-none-build ${CMAKE_COMMAND} --build .)

# Check that we get a warning when the project sets the policy before find_package.
set(RunCMake_TEST_NO_CLEAN FALSE)
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/project-policy-old-build")
run_cmake_with_options(project-policy-old ${cmake_opts} -DCMP0156-OLD=TRUE)

# Check that we get do not get a warning when the project force sets the policy to NEW.
set(RunCMake_TEST_NO_CLEAN FALSE)
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/project-policy-new-forced-build")
run_cmake_with_options(project-policy-new-forced ${cmake_opts} -DFORCE-CMP0156-NEW=TRUE)


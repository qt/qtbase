include(QtRunCMake)

set(RunCMake_TEST_OUTPUT_MERGE TRUE)

set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/build")
set(install_dir "${RunCMake_TEST_BINARY_DIR}/install")

run_cmake_with_options(deploy-extra-plugin
    "-DQt6_DIR=${Qt6_DIR}"
    "-DCMAKE_INSTALL_PREFIX=${install_dir}"
    "-DCMAKE_BUILD_TYPE=Release"
)

# Do not remove the current RunCMake_TEST_BINARY_DIR for the next operations.
set(RunCMake_TEST_NO_CLEAN TRUE)

run_cmake_command(deploy-extra-plugin-build
    "${CMAKE_COMMAND}" --build . --config Release)

# Installation runs windeployqt, which deploys the windows platform plugin over the copy that was
# pre-installed by the project.
# Assert that the install succeeds and that removing the file berore overwiting it succeeds.
set(RunCMake_TEST_NOT_EXPECT_stdout "Cannot remove existing file")
run_cmake_command(deploy-extra-plugin-install
    "${CMAKE_COMMAND}" --install . --config Release)

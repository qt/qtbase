include(QtRunCMake)

set(RunCMake_TEST_OUTPUT_MERGE TRUE)

# Without the policy, the old unquoted behavior has to be kept.
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/deploy-tool-options-policy-unset-build")
run_cmake_with_options(deploy-tool-options-policy-unset
    "-DQt6_DIR=${Qt6_DIR}"
    "-DCMAKE_BUILD_TYPE=Release"
)

# Opting in to QTP0007 must not warn about it.
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/deploy-tool-options-policy-set-build")
set(RunCMake_TEST_NOT_EXPECT_stdout "QTP0007")
run_cmake_with_options(deploy-tool-options-policy-set
    "-DQt6_DIR=${Qt6_DIR}"
    "-DCMAKE_BUILD_TYPE=Release"
)
unset(RunCMake_TEST_NOT_EXPECT_stdout)

# Expected to fail, see comment inside.
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/deploy-tool-options-versionless-build")
run_cmake_with_options(deploy-tool-options-versionless
    "-DQt6_DIR=${Qt6_DIR}"
    "-DCMAKE_BUILD_TYPE=Release"
)


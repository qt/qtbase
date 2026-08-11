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

# The version-less command has to behave like the 'qt6_'-prefixed one.
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/versionless-deploy-tool-options-build")
run_cmake_with_options(versionless-deploy-tool-options
    "-DQt6_DIR=${Qt6_DIR}"
    "-DCMAKE_BUILD_TYPE=Release"
)

# Same values again, but with the deprecated macro implementation, which is expected to fail.
set(RunCMake_TEST_BINARY_DIR
    "${RunCMake_BINARY_DIR}/versionless-deploy-tool-options-deprecated-macro-build")
run_cmake_with_options(versionless-deploy-tool-options-deprecated-macro
    "-DQt6_DIR=${Qt6_DIR}"
    "-DCMAKE_BUILD_TYPE=Release"
)

# Check that the version-less qt_generate_deploy_script command handles
# backslashes and other special characters in arguments correctly.
set(RunCMake_TEST_BINARY_DIR
    "${RunCMake_BINARY_DIR}/versionless-arguments-build")
run_cmake_with_options(versionless-arguments
    "-DQt6_DIR=${Qt6_DIR}"
    "-DCMAKE_BUILD_TYPE=Release"
)

# Check the OLD behavior of QTP0008, where the arguments are re-evaluated.
set(RunCMake_TEST_BINARY_DIR
    "${RunCMake_BINARY_DIR}/versionless-arguments-deprecated-macro-build")
run_cmake_with_options(versionless-arguments-deprecated-macro
    "-DQt6_DIR=${Qt6_DIR}"
    "-DCMAKE_BUILD_TYPE=Release"
)

# Same, with the deprecation warning opted out.
set(RunCMake_TEST_NOT_EXPECT_stdout "QTP0008 is not set")
set(RunCMake_TEST_BINARY_DIR
    "${RunCMake_BINARY_DIR}/versionless-deprecated-macro-no-warning-build")
run_cmake_with_options(versionless-deprecated-macro-no-warning
    "-DQt6_DIR=${Qt6_DIR}"
    "-DCMAKE_BUILD_TYPE=Release"
)
unset(RunCMake_TEST_NOT_EXPECT_stdout)

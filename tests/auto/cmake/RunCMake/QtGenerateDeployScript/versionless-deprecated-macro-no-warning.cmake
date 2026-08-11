find_package(Qt6 REQUIRED COMPONENTS Core)

# Asking for OLD explicitly has to be silent, unlike leaving the policy unset.
qt_policy(SET QTP0008 OLD)

qt_add_executable(app MACOSX_BUNDLE main.cpp)
target_link_libraries(app PRIVATE Qt6::Core)

# Same as versionless-arguments-deprecated-macro. Asking for OLD explicitly must not produce
# the warning about the policy being unset, which RunCMake_TEST_NOT_EXPECT_stdout asserts.
qt_generate_deploy_app_script(
    TARGET app
    OUTPUT_SCRIPT deploy_script
    DEPLOY_TOOL_OPTIONS
        "-doubled=a\\\\b"
)

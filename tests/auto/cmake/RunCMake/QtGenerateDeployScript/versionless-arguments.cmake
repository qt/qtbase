find_package(Qt6 REQUIRED COMPONENTS Core)

# Ask for the new argument forwarding, the values have to arrive unchanged.
qt_policy(SET QTP0008 NEW)

# Use the version-less command on purpose. The values below are the ones that a
# macro wrapper forwarding ${ARGV} would cause issues with: a backslash is consumed as an
# escape and a ${} reference is expanded in the caller's scope.
qt_generate_deploy_script(
    NAME fidelity
    OUTPUT_SCRIPT deploy_script
    CONTENT "\
# backslash: a\\b
# regex: foo\\.dylib
# quote: say \"hi\"
# dollar: \${QT_DEPLOY_PREFIX}
"
)

if(NOT deploy_script)
    message(FATAL_ERROR
        "qt_generate_deploy_script() did not set OUTPUT_SCRIPT in the caller's scope.")
endif()

qt_add_executable(app MACOSX_BUNDLE main.cpp)
target_link_libraries(app PRIVATE Qt6::Core)

# Both the single and doubled backslashes have to be in the generated script as-is.
qt_generate_deploy_app_script(
    TARGET app
    OUTPUT_SCRIPT app_deploy_script
    DEPLOY_TOOL_OPTIONS
        "-backslash=a\\b"
        "-doubled=a\\\\b"
)

if(NOT app_deploy_script)
    message(FATAL_ERROR
        "qt_generate_deploy_app_script() did not set OUTPUT_SCRIPT in the caller's scope.")
endif()

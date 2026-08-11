find_package(Qt6 REQUIRED COMPONENTS Core)

# Ask for the old argument forwarding.
qt_policy(SET QTP0008 OLD)

qt_add_executable(app MACOSX_BUNDLE main.cpp)
target_link_libraries(app PRIVATE Qt6::Core)

# This is what a project would use if it worked around the macro implementation by doubling its
# backslashes.
# The four backslashes below become two when cmake unescapes the quoted argument, and the macro's
# argument substitution consumes one more level, so the value reaches the command as a\b, which is
# what such a project intends.
# The function implementation does not consume one level, so
# the same source would arrive as a\\b.
# Projects in that situation set QTP0008 to OLD until they
# can drop the extra escaping.
qt_generate_deploy_app_script(
    TARGET app
    OUTPUT_SCRIPT deploy_script
    DEPLOY_TOOL_OPTIONS
        "-doubled=a\\\\b"
)

if(NOT deploy_script)
    message(FATAL_ERROR
        "qt_generate_deploy_app_script() did not set OUTPUT_SCRIPT in the caller's scope.")
endif()

# A second call, so that the check script can assert the deprecation warning is
# shown once per cmake run rather than once per call.
qt_generate_deploy_script(
    NAME second
    OUTPUT_SCRIPT second_script
    CONTENT "# nothing to deploy
"
)

if(NOT second_script)
    message(FATAL_ERROR
        "qt_generate_deploy_script() did not set OUTPUT_SCRIPT in the caller's scope.")
endif()

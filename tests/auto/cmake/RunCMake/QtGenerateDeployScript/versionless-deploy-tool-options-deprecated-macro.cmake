# cmake 4.4 added CMP0219, whose NEW behavior preserves backslashes in macro arguments and
# thus would make the call below succeed. Set it to OLD, so that the case keeps checking
# what the macro implementation does. Older cmake versions only have the OLD behavior.
if(POLICY CMP0219)
    cmake_policy(SET CMP0219 OLD)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core)

# Ask for the old argument forwarding.
qt_policy(SET QTP0008 OLD)

# Opt in to the quoted argument values.
qt_policy(SET QTP0007 NEW)

qt_add_executable(app MACOSX_BUNDLE main.cpp)
target_link_libraries(app PRIVATE Qt6::Core)

# Same as the versionless-deploy-tool-options case, but with the deprecated macro
# implementation of the version-less command, which is selected by setting
# QTP0008 to OLD.
# The macro forwards ${ARGV}, which cmake re-parses, so it consumes one level of
# backslashes and '\b' is not a valid escape sequence. Assert that this still fails, which
# is why the macro implementation is deprecated.
qt_generate_deploy_app_script(
    TARGET app
    OUTPUT_SCRIPT deploy_script
    DEPLOY_TOOL_OPTIONS
        "-codesign=Developer ID Application: ACME & Co. (ABC123)"
        "-quote=say \"hi\""
        "-backslash=a\\b"
        "-double-backslash=a\\\\b"
        "-backslash-quote=a\\\"b"
        "-genex=$<1:expanded>"
        "-trailing-backslash=a\\"
    POST_EXCLUDE_FILES
        "/some path/with spaces/lib.so"
)

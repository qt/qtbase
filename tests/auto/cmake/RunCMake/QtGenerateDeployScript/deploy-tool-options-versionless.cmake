# cmake 4.4 added CMP0219, whose NEW behavior preserves backslashes in macro
# arguments and thus would make the call below succeed. Set it to OLD to test the old
# cmake behavior with the NEW behavior of the qt policy.
if(POLICY CMP0219)
    cmake_policy(SET CMP0219 OLD)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core)

# Opt in to the quoted argument values.
qt_policy(SET QTP0007 NEW)

qt_add_executable(app MACOSX_BUNDLE main.cpp)
target_link_libraries(app PRIVATE Qt6::Core)

# Test behavior of the version-less command.
# It is a macro that forwards ${ARGV}, which cmake re-parses and thus consumes one level of
# backslashes. This results in an invalid escape sequence '\b' which will fail generation.
# Assert that this fails until the macro is fixed.
# Note that setting QTP0007 does not help, because the values are already mangled before the
# policy is considered.
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

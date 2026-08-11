include("${CMAKE_CURRENT_LIST_DIR}/cmake/DeployScriptTestCheckHelpers.cmake")

# The OLD behavior writes the options and their values as one unquoted semicolon
# separated argument, which is why the generated script splits them at whitespace when it
# is executed.
set(expected_values
    DEPLOY_TOOL_OPTIONS
    [==[-codesign=Developer ID Application: ACME & Co. (ABC123)]==]
    [==[-quote=say "hi"]==]
    [==[-backslash=a\b]==]
    [==[-double-backslash=a\\b]==]
    [==[-backslash-quote=a\"b]==]
    [==[-genex=expanded]==]
    [==[-trailing-backslash=a]==]
    POST_EXCLUDE_FILES
    [==[/some path/with spaces/lib.so)]==]
)
list(JOIN expected_values ";" expected)

deploy_script_assert_contains("${expected}" "the unquoted arguments of the OLD behavior")

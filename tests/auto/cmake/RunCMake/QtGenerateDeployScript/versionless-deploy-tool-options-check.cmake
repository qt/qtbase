include("${CMAKE_CURRENT_LIST_DIR}/cmake/DeployScriptTestCheckHelpers.cmake")

# Same values as the deploy-tool-options-policy-set case, and the same result, except for
# the value that ends in a backslash. Arguments are forwarded in a semicolon separated list,
# so a trailing backslash escapes a separator and is lost no matter which behavior QTP0008
# selects.
# That can only be avoided by calling the 'qt6_'-prefixed command.
set(expected [==[
    DEPLOY_TOOL_OPTIONS
        "-codesign=Developer ID Application: ACME & Co. (ABC123)"
        "-quote=say \"hi\""
        "-backslash=a\\b"
        "-double-backslash=a\\\\b"
        "-backslash-quote=a\\\"b"
        "-genex=expanded"
        "-trailing-backslash=a"
    POST_EXCLUDE_FILES
        "/some path/with spaces/lib.so"
]==])

deploy_script_assert_contains("${expected}" "the expected quoted arguments")

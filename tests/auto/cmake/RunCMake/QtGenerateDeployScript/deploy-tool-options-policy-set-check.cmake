include("${CMAKE_CURRENT_LIST_DIR}/cmake/DeployScriptTestCheckHelpers.cmake")

# Assert that we get the original values verbatim.
set(expected [==[
    DEPLOY_TOOL_OPTIONS
        "-codesign=Developer ID Application: ACME & Co. (ABC123)"
        "-quote=say \"hi\""
        "-backslash=a\\b"
        "-double-backslash=a\\\\b"
        "-backslash-quote=a\\\"b"
        "-genex=expanded"
        "-trailing-backslash=a\\"
    POST_EXCLUDE_FILES
        "/some path/with spaces/lib.so"
]==])

deploy_script_assert_contains("${expected}" "the expected quoted arguments")

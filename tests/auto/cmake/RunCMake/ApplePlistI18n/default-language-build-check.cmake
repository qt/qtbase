execute_process(
    COMMAND /usr/libexec/PlistBuddy -c "print CFBundleDevelopmentRegion"
        "${RunCMake_TEST_BINARY_DIR}/app.app/Contents/Info.plist"
    OUTPUT_VARIABLE dev_region
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    set(RunCMake_TEST_FAILED
        "Failed to read CFBundleDevelopmentRegion from Info.plist")
elseif(NOT dev_region STREQUAL "en")
    set(RunCMake_TEST_FAILED
        "CFBundleDevelopmentRegion is '${dev_region}', expected 'en'")
endif()

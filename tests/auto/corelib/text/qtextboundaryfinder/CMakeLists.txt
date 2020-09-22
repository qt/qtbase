# Generated from qtextboundaryfinder.pro.

#####################################################################
## tst_qtextboundaryfinder Test:
#####################################################################

# Collect test data
list(APPEND test_data "data")

qt_internal_add_test(tst_qtextboundaryfinder
    SOURCES
        tst_qtextboundaryfinder.cpp
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

if(ANDROID AND NOT ANDROID_EMBEDDED)
    # Resources:
    set(testdata_resource_files
        "data/GraphemeBreakTest.txt"
        "data/LineBreakTest.txt"
        "data/SentenceBreakTest.txt"
        "data/WordBreakTest.txt"
    )

    qt_internal_add_resource(tst_qtextboundaryfinder "testdata"
        PREFIX
            "/"
        FILES
            ${testdata_resource_files}
    )
endif()

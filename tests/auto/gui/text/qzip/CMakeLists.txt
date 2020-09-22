# Generated from qzip.pro.

#####################################################################
## tst_qzip Test:
#####################################################################

# Collect test data
list(APPEND test_data "testdata")

qt_internal_add_test(tst_qzip
    SOURCES
        tst_qzip.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

if(ANDROID)
    # Resources:
    set(testdata_resource_files
        "testdata/symlink.zip"
        "testdata/test.zip"
    )

    qt_internal_add_resource(tst_qzip "testdata"
        PREFIX
            "/"
        FILES
            ${testdata_resource_files}
    )
endif()

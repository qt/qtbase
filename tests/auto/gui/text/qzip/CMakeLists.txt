# Generated from qzip.pro.

#####################################################################
## tst_qzip Test:
#####################################################################

# Collect test data
list(APPEND test_data "testdata")

add_qt_test(tst_qzip
    SOURCES
        tst_qzip.cpp
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
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

    add_qt_resource(tst_qzip "testdata"
        PREFIX
            "/"
        FILES
            ${testdata_resource_files}
    )
endif()

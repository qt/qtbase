# Generated from qcssparser.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qcssparser Test:
#####################################################################

# Collect test data
list(APPEND test_data "testdata")

qt_internal_add_test(tst_qcssparser
    SOURCES
        tst_qcssparser.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Xml
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qcssparser.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"

## Scopes:
#####################################################################

if(ANDROID)
    # Resources:
    set(testdata_resource_files
        "testdata/scanner/comments/input"
        "testdata/scanner/comments/output"
        "testdata/scanner/comments2/input"
        "testdata/scanner/comments2/output"
        "testdata/scanner/comments3/input"
        "testdata/scanner/comments3/output"
        "testdata/scanner/comments4/input"
        "testdata/scanner/comments4/output"
        "testdata/scanner/quotedstring/input"
        "testdata/scanner/quotedstring/output"
        "testdata/scanner/simple/input"
        "testdata/scanner/simple/output"
        "testdata/scanner/unicode/input"
        "testdata/scanner/unicode/output"
    )

    qt_internal_add_resource(tst_qcssparser "testdata"
        PREFIX
            "/"
        FILES
            ${testdata_resource_files}
    )
endif()

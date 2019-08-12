# Generated from qtemporaryfile.pro.

#####################################################################
## tst_qtemporaryfile Test:
#####################################################################

# Collect test data
list(APPEND test_data "tst_qtemporaryfile.cpp")

add_qt_test(tst_qtemporaryfile
    GUI
    SOURCES
        tst_qtemporaryfile.cpp
    LIBRARIES
        Qt::TestPrivate
    TESTDATA ${test_data}
)

# Resources:
add_qt_resource(tst_qtemporaryfile "qtemporaryfile"
    PREFIX
        "/"
    FILES
        "resources/test.txt"
)


## Scopes:
#####################################################################

# Resources:
add_qt_resource(tst_qtemporaryfile "android_testdata"
    PREFIX
        "/android_testdata"
    FILES
        "resources/test.txt"
        "tst_qtemporaryfile.cpp"
)


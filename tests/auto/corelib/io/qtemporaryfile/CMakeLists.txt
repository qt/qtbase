# Generated from qtemporaryfile.pro.

#####################################################################
## tst_qtemporaryfile Test:
#####################################################################

add_qt_test(tst_qtemporaryfile
    SOURCES
        tst_qtemporaryfile.cpp
    LIBRARIES
        Qt::TestPrivate
)

# Resources:
add_qt_resource(tst_qtemporaryfile "qtemporaryfile" FILES
    resources/test.txt)


#### Keys ignored in scope 1:.:.:qtemporaryfile.pro:<TRUE>:
# CONFIG = "testcase"
# TESTDATA = "tst_qtemporaryfile.cpp"

## Scopes:
#####################################################################

# Resources:
add_qt_resource(tst_qtemporaryfile "android_testdata" PREFIX "/android_testdata" FILES
    resources/test.txt
    tst_qtemporaryfile.cpp)


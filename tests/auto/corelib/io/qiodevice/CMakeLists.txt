# Generated from qiodevice.pro.

#####################################################################
## tst_qiodevice Test:
#####################################################################

add_qt_test(tst_qiodevice
    SOURCES
        tst_qiodevice.cpp
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:qiodevice.pro:<TRUE>:
# CONFIG = "testcase"
# MOC_DIR = "tmp"
# TESTDATA = "tst_qiodevice.cpp"

## Scopes:
#####################################################################

# Resources:
add_qt_resource(tst_qiodevice "android_testdata" PREFIX "/" FILES
    tst_qiodevice.cpp)


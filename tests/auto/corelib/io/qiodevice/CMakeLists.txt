# Generated from qiodevice.pro.

#####################################################################
## tst_qiodevice Test:
#####################################################################

# Collect test data
list(APPEND test_data "tst_qiodevice.cpp")

add_qt_test(tst_qiodevice
    GUI
    SOURCES
        tst_qiodevice.cpp
    PUBLIC_LIBRARIES
        Qt::Network
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qiodevice.pro:<TRUE>:
# MOC_DIR = "tmp"

## Scopes:
#####################################################################

# Resources:
add_qt_resource(tst_qiodevice "android_testdata"
    PREFIX
        "/"
    FILES
        "tst_qiodevice.cpp"
)


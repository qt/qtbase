# Generated from qloggingregistry.pro.

#####################################################################
## tst_qloggingregistry Test:
#####################################################################

# Collect test data
list(APPEND test_data "qtlogging.ini")

add_qt_test(tst_qloggingregistry
    GUI
    SOURCES
        tst_qloggingregistry.cpp
    LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qloggingregistry.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

# Resources:
add_qt_resource(tst_qloggingregistry "android_testdata"
    PREFIX
        "/"
    FILES
        "qtlogging.ini"
)


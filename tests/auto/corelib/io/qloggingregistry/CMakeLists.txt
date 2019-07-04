# Generated from qloggingregistry.pro.

#####################################################################
## tst_qloggingregistry Test:
#####################################################################

add_qt_test(tst_qloggingregistry
    SOURCES
        tst_qloggingregistry.cpp
    LIBRARIES
        Qt::CorePrivate
)

#### Keys ignored in scope 1:.:.:qloggingregistry.pro:<TRUE>:
# CONFIG = "testcase"
# TEMPLATE = "app"
# TESTDATA = "qtlogging.ini"

## Scopes:
#####################################################################

# Resources:
add_qt_resource(tst_qloggingregistry "android_testdata" PREFIX "/" FILES
    qtlogging.ini)


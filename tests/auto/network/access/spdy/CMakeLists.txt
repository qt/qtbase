# Generated from spdy.pro.

#####################################################################
## tst_spdy Test:
#####################################################################

add_qt_test(tst_spdy
    GUI
    SOURCES
        tst_spdy.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:spdy.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:spdy.pro:WIN32:
# CONFIG = "insignificant_test"

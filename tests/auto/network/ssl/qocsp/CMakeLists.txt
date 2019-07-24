# Generated from qocsp.pro.

#####################################################################
## tst_qocsp Test:
#####################################################################

add_qt_test(tst_qocsp
    GUI
    SOURCES
        tst_qocsp.cpp
    LIBRARIES
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:qocsp.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:qocsp.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "debug"

#### Keys ignored in scope 4:.:.:qocsp.pro:else:
# DESTDIR = "release"

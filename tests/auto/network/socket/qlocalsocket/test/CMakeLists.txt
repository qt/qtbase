# Generated from test.pro.

#####################################################################
## tst_qlocalsocket Test:
#####################################################################

qt_add_test(tst_qlocalsocket
    SOURCES
        ../tst_qlocalsocket.cpp
    DEFINES
        QLOCALSERVER_DEBUG
        QLOCALSOCKET_DEBUG
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/../\\\"
    PUBLIC_LIBRARIES
        Qt::Network
)

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:test.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "../debug"

#### Keys ignored in scope 4:.:.:test.pro:else:
# DESTDIR = "../release"

#### Keys ignored in scope 5:.:.:test.pro:else:
# DESTDIR = ".."

# Generated from test.pro.

#####################################################################
## tst_qnetworksession Test:
#####################################################################

add_qt_test(tst_qnetworksession
    GUI
    SOURCES
        ../../qbearertestcommon.h
        tst_qnetworksession.cpp
    LIBRARIES
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:test.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:test.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "../debug"

#### Keys ignored in scope 4:.:.:test.pro:else:
# DESTDIR = "../release"

#### Keys ignored in scope 5:.:.:test.pro:else:
# DESTDIR = ".."

#### Keys ignored in scope 6:.:.:test.pro:NOT ANDROID AND NOT WINRT:
# TEST_HELPER_INSTALLS = "../lackey/lackey"

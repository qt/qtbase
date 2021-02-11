# Generated from test.pro.

#####################################################################
## tst_qudpsocket Test:
#####################################################################

qt_internal_add_test(tst_qudpsocket
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../" # special case
    SOURCES
        ../tst_qudpsocket.cpp
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::TestPrivate
    QT_TEST_SERVER_LIST "danted" "echo" # special case
)

#### Keys ignored in scope 1:.:.:test.pro:<TRUE>:
# MOC_DIR = "tmp"
# testcase.timeout = "800"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:test.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "../debug"

#### Keys ignored in scope 4:.:.:test.pro:else:
# DESTDIR = "../release"

#### Keys ignored in scope 5:.:.:test.pro:else:
# DESTDIR = "../"

#### Keys ignored in scope 6:.:.:test.pro:LINUX:
# QT_TEST_SERVER_LIST = "danted" "echo"

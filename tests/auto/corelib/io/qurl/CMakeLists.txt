# Generated from qurl.pro.

#####################################################################
## tst_qurl Test:
#####################################################################

add_qt_test(tst_qurl
    SOURCES
        tst_qurl.cpp
    PUBLIC_LIBRARIES
        Qt::Concurrent
)

#### Keys ignored in scope 1:.:.:qurl.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

extend_target(tst_qurl CONDITION APPLE_OSX
    SOURCES
        tst_qurl_mac.mm
)

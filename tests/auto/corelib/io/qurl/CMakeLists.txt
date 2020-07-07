# Generated from qurl.pro.

#####################################################################
## tst_qurl Test:
#####################################################################

qt_add_test(tst_qurl
    SOURCES
        tst_qurl.cpp
    PUBLIC_LIBRARIES
        Qt::Concurrent
)

## Scopes:
#####################################################################

qt_extend_target(tst_qurl CONDITION APPLE
    SOURCES
        tst_qurl_mac.mm
)

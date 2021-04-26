# Generated from qurl.pro.

#####################################################################
## tst_qurl Test:
#####################################################################

qt_internal_add_test(tst_qurl
    SOURCES
        tst_qurl.cpp
    PUBLIC_LIBRARIES
        Qt::Concurrent
        Qt::TestPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qurl CONDITION APPLE
    SOURCES
        tst_qurl_mac.mm
)

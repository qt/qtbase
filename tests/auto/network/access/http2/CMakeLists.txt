# Generated from http2.pro.

#####################################################################
## tst_http2 Test:
#####################################################################

qt_internal_add_test(tst_http2
    SOURCES
        http2srv.cpp http2srv.h
        tst_http2.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
        Qt::TestPrivate
)

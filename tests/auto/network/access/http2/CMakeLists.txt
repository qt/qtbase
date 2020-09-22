# Generated from http2.pro.

#####################################################################
## tst_http2 Test:
#####################################################################

qt_internal_add_test(tst_http2
    SOURCES
        ../../../../shared/emulationdetector.h
        http2srv.cpp http2srv.h
        tst_http2.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    INCLUDE_DIRECTORIES
        ../../../../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
)

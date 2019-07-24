# Generated from http2.pro.

#####################################################################
## tst_http2 Test:
#####################################################################

add_qt_test(tst_http2
    GUI
    SOURCES
        ../../../../shared/emulationdetector.h
        http2srv.cpp http2srv.h
        tst_http2.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    INCLUDE_DIRECTORIES
        ../../../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:http2.pro:<TRUE>:
# CONFIG = "testcase" "parallel_test" "c++11"

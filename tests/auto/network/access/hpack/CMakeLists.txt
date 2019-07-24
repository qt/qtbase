# Generated from hpack.pro.

#####################################################################
## tst_hpack Test:
#####################################################################

add_qt_test(tst_hpack
    GUI
    SOURCES
        tst_hpack.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:hpack.pro:<TRUE>:
# CONFIG = "testcase" "parallel_test" "c++11"
# TEMPLATE = "app"

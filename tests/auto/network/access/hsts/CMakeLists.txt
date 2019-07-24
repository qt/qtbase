# Generated from hsts.pro.

#####################################################################
## tst_qhsts Test:
#####################################################################

add_qt_test(tst_qhsts
    GUI
    SOURCES
        tst_qhsts.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:hsts.pro:<TRUE>:
# CONFIG = "testcase" "parallel_test" "c++11"
# TEMPLATE = "app"

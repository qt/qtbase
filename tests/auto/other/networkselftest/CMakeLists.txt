# Generated from networkselftest.pro.

#####################################################################
## tst_networkselftest Test:
#####################################################################

add_qt_test(tst_networkselftest
    SOURCES
        tst_networkselftest.cpp
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

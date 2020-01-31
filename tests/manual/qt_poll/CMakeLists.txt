# Generated from qt_poll.pro.

#####################################################################
## tst_qt_poll Test:
#####################################################################

add_qt_test(tst_qt_poll
    SOURCES
        ../../../src/corelib/kernel/qpoll.cpp
        tst_qt_poll.cpp
    INCLUDE_DIRECTORIES
        ../../../src/corelib/kernel
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

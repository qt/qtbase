# Generated from qt_poll.pro.

#####################################################################
## tst_qt_poll Test:
#####################################################################

qt_internal_add_test(tst_qt_poll
    SOURCES
        ../../../src/corelib/kernel/qpoll.cpp
        tst_qt_poll.cpp
    INCLUDE_DIRECTORIES
        ../../../src/corelib/kernel
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Network
)

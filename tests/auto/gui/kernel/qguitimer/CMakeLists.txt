# Generated from qguitimer.pro.

#####################################################################
## tst_qguitimer Test:
#####################################################################

add_qt_test(tst_qguitimer
    SOURCES
        ../../../corelib/kernel/qtimer/tst_qtimer.cpp
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Threads::Threads # special case
)

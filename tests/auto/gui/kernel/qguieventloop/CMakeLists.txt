# Generated from qguieventloop.pro.

#####################################################################
## tst_qguieventloop Test:
#####################################################################

add_qt_test(tst_qguieventloop
    GUI
    SOURCES
        ../../../corelib/kernel/qeventloop/tst_qeventloop.cpp
    LIBRARIES # special case
        Qt::CorePrivate # special case
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network # special case
)

## Scopes:
#####################################################################

extend_target(tst_qguieventloop CONDITION WIN32 AND NOT WINRT
    PUBLIC_LIBRARIES
        user32
)

extend_target(tst_qguieventloop CONDITION QT_FEATURE_glib
    DEFINES
        HAVE_GLIB
)

# Generated from qguieventloop.pro.

#####################################################################
## tst_qguieventloop Test:
#####################################################################

qt_internal_add_test(tst_qguieventloop
    SOURCES
        ../../../corelib/kernel/qeventloop/tst_qeventloop.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Network
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qguieventloop CONDITION WIN32
    PUBLIC_LIBRARIES
        user32
)

qt_internal_extend_target(tst_qguieventloop CONDITION QT_FEATURE_glib
    DEFINES
        HAVE_GLIB
)

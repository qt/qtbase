# Generated from qeventloop.pro.

#####################################################################
## tst_qeventloop Test:
#####################################################################

qt_internal_add_test(tst_qeventloop
    SOURCES
        tst_qeventloop.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Network
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qeventloop CONDITION WIN32
    PUBLIC_LIBRARIES
        user32
)

qt_internal_extend_target(tst_qeventloop CONDITION QT_FEATURE_glib
    DEFINES
        HAVE_GLIB
)

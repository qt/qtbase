# Generated from qeventloop.pro.

#####################################################################
## tst_qeventloop Test:
#####################################################################

add_qt_test(tst_qeventloop
    SOURCES
        tst_qeventloop.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Network
)

## Scopes:
#####################################################################

extend_target(tst_qeventloop CONDITION WIN32
    PUBLIC_LIBRARIES
        user32
)

extend_target(tst_qeventloop CONDITION QT_FEATURE_glib
    DEFINES
        HAVE_GLIB
)

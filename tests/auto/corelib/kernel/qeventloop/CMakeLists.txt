# Generated from qeventloop.pro.

#####################################################################
## tst_qeventloop Test:
#####################################################################

add_qt_test(tst_qeventloop
    SOURCES
        tst_qeventloop.cpp
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

## Scopes:
#####################################################################

extend_target(tst_qeventloop CONDITION WIN32 AND NOT WINRT
    PUBLIC_LIBRARIES
        user32
)

extend_target(tst_qeventloop CONDITION QT_FEATURE_glib
    DEFINES
        HAVE_GLIB
)

# Generated from qobject.pro.

#####################################################################
## tst_qobject Test:
#####################################################################

add_qt_test(tst_qobject
    SOURCES
        tst_qobject.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Network
)

## Scopes:
#####################################################################
add_subdirectory(signalbug)

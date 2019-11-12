# Generated from qgraphicsitem.pro.

#####################################################################
## tst_qgraphicsitem Test:
#####################################################################

add_qt_test(tst_qgraphicsitem
    SOURCES
        tst_qgraphicsitem.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

## Scopes:
#####################################################################

extend_target(tst_qgraphicsitem CONDITION WIN32 AND NOT WINRT
    PUBLIC_LIBRARIES
        user32
)

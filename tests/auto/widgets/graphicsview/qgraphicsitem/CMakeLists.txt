# Generated from qgraphicsitem.pro.

#####################################################################
## tst_qgraphicsitem Test:
#####################################################################

qt_internal_add_test(tst_qgraphicsitem
    SOURCES
        tst_qgraphicsitem.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qgraphicsitem CONDITION WIN32
    LIBRARIES
        user32
)

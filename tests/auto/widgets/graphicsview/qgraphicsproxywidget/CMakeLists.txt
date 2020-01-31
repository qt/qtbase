# Generated from qgraphicsproxywidget.pro.

#####################################################################
## tst_qgraphicsproxywidget Test:
#####################################################################

add_qt_test(tst_qgraphicsproxywidget
    SOURCES
        tst_qgraphicsproxywidget.cpp
    DEFINES
        QTEST_QPA_MOUSE_HANDLING
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

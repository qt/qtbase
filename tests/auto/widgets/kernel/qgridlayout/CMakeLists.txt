# Generated from qgridlayout.pro.

#####################################################################
## tst_qgridlayout Test:
#####################################################################

add_qt_test(tst_qgridlayout
    SOURCES
        sortdialog.ui
        tst_qgridlayout.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

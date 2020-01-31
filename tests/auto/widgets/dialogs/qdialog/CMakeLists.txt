# Generated from qdialog.pro.

#####################################################################
## tst_qdialog Test:
#####################################################################

add_qt_test(tst_qdialog
    SOURCES
        tst_qdialog.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

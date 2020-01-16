# Generated from qopenglwindow.pro.

#####################################################################
## tst_qopenglwindow Test:
#####################################################################

add_qt_test(tst_qopenglwindow
    SOURCES
        tst_qopenglwindow.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::OpenGL
)

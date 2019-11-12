# Generated from qopenglcontext.pro.

#####################################################################
## qopenglcontext Binary:
#####################################################################

add_qt_manual_test(qopenglcontext
    GUI
    SOURCES
        main.cpp
        qopenglcontextwindow.cpp qopenglcontextwindow.h
    LIBRARIES
        Qt::EglSupportPrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::EglSupport
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:qopenglcontext.pro:<TRUE>:
# TEMPLATE = "app"

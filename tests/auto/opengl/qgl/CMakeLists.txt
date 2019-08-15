# Generated from qgl.pro.

#####################################################################
## tst_qgl Test:
#####################################################################

add_qt_test(tst_qgl
    GUI
    SOURCES
        tst_qgl.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::OpenGLPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::OpenGL
        Qt::Widgets
)

# Resources:
set_source_files_properties("../../gui/image/qpixmap/images/designer.png"
    PROPERTIES QT_RESOURCE_ALIAS "designer.png"
)
add_qt_resource(tst_qgl "qgl"
    PREFIX
        "/"
    FILES
        ../../gui/image/qpixmap/images/designer.png
)


#### Keys ignored in scope 1:.:.:qgl.pro:<TRUE>:
# CONFIG = "testcase"

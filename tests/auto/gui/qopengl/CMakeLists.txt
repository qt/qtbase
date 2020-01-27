# Generated from qopengl.pro.

#####################################################################
## tst_qopengl Test:
#####################################################################

add_qt_test(tst_qopengl
    SOURCES
        tst_qopengl.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::OpenGL
)

## Scopes:
#####################################################################

extend_target(tst_qopengl CONDITION LINUX AND QT_FEATURE_xcb AND QT_FEATURE_xcb_glx_plugin
    DEFINES
        USE_GLX
)

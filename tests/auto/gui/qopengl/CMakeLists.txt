# Generated from qopengl.pro.

#####################################################################
## tst_qopengl Test:
#####################################################################

qt_internal_add_test(tst_qopengl
    SOURCES
        tst_qopengl.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGL
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qopengl CONDITION LINUX AND QT_FEATURE_xcb AND QT_FEATURE_xcb_glx_plugin
    DEFINES
        USE_GLX
)

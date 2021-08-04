# Generated from minimalegl.pro.
qt_find_package(EGL) # special case

#####################################################################
## QMinimalEglIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QMinimalEglIntegrationPlugin
    OUTPUT_NAME qminimalegl
    PLUGIN_TYPE platforms
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES minimalegl # special case
    SOURCES
        main.cpp
        qminimaleglintegration.cpp qminimaleglintegration.h
        qminimaleglscreen.cpp qminimaleglscreen.h
        qminimaleglwindow.cpp qminimaleglwindow.h
    DEFINES
        QT_EGL_NO_X11
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        EGL::EGL # special case
)

#### Keys ignored in scope 1:.:.:minimalegl.pro:<TRUE>:
# OTHER_FILES = "minimalegl.json"

## Scopes:
#####################################################################

qt_internal_extend_target(QMinimalEglIntegrationPlugin CONDITION QT_FEATURE_opengl
    SOURCES
        qminimaleglbackingstore.cpp qminimaleglbackingstore.h
    LIBRARIES
        Qt::OpenGL
)

#### Keys ignored in scope 3:.:.:minimalegl.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"

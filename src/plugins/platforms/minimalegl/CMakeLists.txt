# Generated from minimalegl.pro.

#####################################################################
## QMinimalEglIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QMinimalEglIntegrationPlugin
    OUTPUT_NAME qminimalegl
    TYPE platforms
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES minimalegl # special case
    SOURCES
        main.cpp
        qminimaleglintegration.cpp qminimaleglintegration.h
        qminimaleglscreen.cpp qminimaleglscreen.h
        qminimaleglwindow.cpp qminimaleglwindow.h
    DEFINES
        QT_EGL_NO_X11
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::EglSupportPrivate
        Qt::EventDispatcherSupportPrivate
        Qt::FontDatabaseSupportPrivate
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:minimalegl.pro:<TRUE>:
# OTHER_FILES = "minimalegl.json"

## Scopes:
#####################################################################

qt_extend_target(QMinimalEglIntegrationPlugin CONDITION QT_FEATURE_opengl
    SOURCES
        qminimaleglbackingstore.cpp qminimaleglbackingstore.h
    PUBLIC_LIBRARIES
        Qt::OpenGL
)

#### Keys ignored in scope 3:.:.:minimalegl.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"

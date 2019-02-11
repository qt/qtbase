# Generated from minimalegl.pro.

#####################################################################
## qminimalegl Plugin:
#####################################################################

add_qt_plugin(qminimalegl
    TYPE platforms
    SOURCES
        main.cpp
        qminimaleglintegration.cpp qminimaleglintegration.h
        qminimaleglscreen.cpp qminimaleglscreen.h
        qminimaleglwindow.cpp qminimaleglwindow.h
    DEFINES
        QT_EGL_NO_X11
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::EventDispatcherSupportPrivate
        Qt::FontDatabaseSupportPrivate
        Qt::EglSupportPrivate
    # CONFIG = "egl"
    # OTHER_FILES = "minimalegl.json"
    # PLUGIN_CLASS_NAME = "QMinimalEglIntegrationPlugin"
    # _LOADED = "qt_plugin"
)

## Scopes:
#####################################################################

extend_target(qminimalegl CONDITION QT_FEATURE_opengl
    SOURCES
        qminimaleglbackingstore.cpp qminimaleglbackingstore.h
)
#
#extend_target(qminimalegl CONDITION NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN
#    # PLUGIN_EXTENDS = "-"
#)

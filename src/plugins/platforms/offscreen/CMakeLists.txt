# Generated from offscreen.pro.

find_package(X11)

#####################################################################
## qoffscreen Plugin:
#####################################################################

add_qt_plugin(qoffscreen
    TYPE platforms
    SOURCES
        main.cpp
        qoffscreencommon.cpp qoffscreencommon.h
        qoffscreenintegration.cpp qoffscreenintegration.h
        qoffscreenwindow.cpp qoffscreenwindow.h
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::EventDispatcherSupportPrivate
        Qt::FontDatabaseSupportPrivate
    # OTHER_FILES = "offscreen.json"
    # PLUGIN_CLASS_NAME = "QOffscreenIntegrationPlugin"
    # _LOADED = "qt_plugin"
)

## Scopes:
#####################################################################

extend_target(qoffscreen CONDITION QT_FEATURE_opengl AND QT_FEATURE_xlib AND NOT QT_FEATURE_opengles2
    SOURCES
        qoffscreenintegration_x11.cpp qoffscreenintegration_x11.h
    LIBRARIES
        Qt::GlxSupportPrivate
)

extend_target(qoffscreen CONDITION QT_FEATURE_opengl AND QT_FEATURE_opengles2 AND QT_FEATURE_xlib
    SOURCES
        qoffscreenintegration_dummy.cpp
)
#
#extend_target(qoffscreen CONDITION NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN
#    # PLUGIN_EXTENDS = "-"
#)

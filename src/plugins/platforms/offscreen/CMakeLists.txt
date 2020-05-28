# Generated from offscreen.pro.

#####################################################################
## QOffscreenIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QOffscreenIntegrationPlugin
    OUTPUT_NAME qoffscreen
    TYPE platforms
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES offscreen # special case
    SOURCES
        main.cpp
        qoffscreencommon.cpp qoffscreencommon.h
        qoffscreenintegration.cpp qoffscreenintegration.h
        qoffscreenwindow.cpp qoffscreenwindow.h
    DEFINES
        QT_NO_FOREACH
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:offscreen.pro:<TRUE>:
# OTHER_FILES = "offscreen.json"

## Scopes:
#####################################################################

qt_extend_target(QOffscreenIntegrationPlugin CONDITION UNIX
    PUBLIC_LIBRARIES
        Qt::EventDispatcherSupportPrivate
)

qt_extend_target(QOffscreenIntegrationPlugin CONDITION NOT APPLE
    PUBLIC_LIBRARIES
        Qt::FontDatabaseSupportPrivate
)

qt_extend_target(QOffscreenIntegrationPlugin CONDITION QT_FEATURE_opengl AND QT_FEATURE_xlib AND NOT QT_FEATURE_opengles2
    SOURCES
        qoffscreenintegration_x11.cpp qoffscreenintegration_x11.h
    PUBLIC_LIBRARIES
        Qt::GlxSupportPrivate
)

#### Keys ignored in scope 5:.:.:offscreen.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"

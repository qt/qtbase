# Generated from minimal.pro.

#####################################################################
## qminimal Plugin:
#####################################################################

add_qt_plugin(qminimal
    TYPE platforms
    CLASS_NAME QMinimalIntegrationPlugin
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES minimal # special case
    SOURCES
        main.cpp
        qminimalbackingstore.cpp qminimalbackingstore.h
        qminimalintegration.cpp qminimalintegration.h
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::CorePrivate
        Qt::EventDispatcherSupportPrivate
        Qt::FontDatabaseSupportPrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::EventDispatcherSupport
        Qt::FontDatabaseSupport
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:minimal.pro:<TRUE>:
# OTHER_FILES = "minimal.json"

## Scopes:
#####################################################################

extend_target(qminimal CONDITION QT_FEATURE_freetype
    LIBRARIES
        WrapFreetype::WrapFreetype
)

#### Keys ignored in scope 3:.:.:minimal.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"

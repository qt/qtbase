qt_find_package(WrapFreetype) # special case

#####################################################################
## qminimal Plugin:
#####################################################################

add_qt_plugin(qminimal
    TYPE platforms
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
)

#### Keys ignored in scope 1:.:minimal.pro:<NONE>:
# OTHER_FILES = "minimal.json"
# PLUGIN_CLASS_NAME = "QMinimalIntegrationPlugin"
# _LOADED = "qt_plugin"

## Scopes:
#####################################################################

extend_target(qminimal CONDITION QT_FEATURE_freetype
    LIBRARIES
        WrapFreetype::WrapFreetype
)

#### Keys ignored in scope 3:.:minimal.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"

# Generated from minimal.pro.

find_package(Freetype)

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
        Qt::GuiPrivate
        Qt::EventDispatcherSupportPrivate
        Qt::FontDatabaseSupportPrivate
    # OTHER_FILES = "minimal.json"
    # PLUGIN_CLASS_NAME = "QMinimalIntegrationPlugin"
    # _LOADED = "qt_plugin"
)

## Scopes:
#####################################################################

extend_target(qminimal CONDITION QT_FEATURE_freetype
    LIBRARIES
        Freetype::Freetype
)
#
#extend_target(qminimal CONDITION NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN
#    # PLUGIN_EXTENDS = "-"
#)

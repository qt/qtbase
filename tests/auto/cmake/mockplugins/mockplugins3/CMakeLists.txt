qt_internal_add_module(MockPlugins3
    PLUGIN_TYPES mockauxplugin
    SOURCES
        fake.cpp
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
)

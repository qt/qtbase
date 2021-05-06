qt_internal_add_module(MockPlugins1
    PLUGIN_TYPES mockplugin
    SOURCES
        fake.cpp
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
)

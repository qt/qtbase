qt_internal_add_plugin(QMock5Plugin
    CLASS_NAME QMock5Plugin
    PLUGIN_TYPE mockplugin
    DEFAULT_IF FALSE
    SOURCES
        qmock5plugin.cpp qmock5plugin.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::MockPlugins3
)

qt_internal_add_plugin(QMock2Plugin
    CLASS_NAME QMock2Plugin
    PLUGIN_TYPE mockplugin
    SOURCES
        qmock2plugin.cpp qmock2plugin.h
    LIBRARIES
        Qt::Core
        Qt::MockPlugins1
)

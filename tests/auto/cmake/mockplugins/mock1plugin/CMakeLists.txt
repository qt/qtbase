qt_internal_add_plugin(QMock1Plugin
    CLASS_NAME QMock1Plugin
    PLUGIN_TYPE mockplugin
    SOURCES
        qmock1plugin.cpp qmock1plugin.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::MockPlugins1
)

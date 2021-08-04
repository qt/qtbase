qt_internal_add_plugin(QMock3Plugin
    CLASS_NAME QMock3Plugin
    PLUGIN_TYPE mockplugin
    SOURCES
        qmock3plugin.cpp qmock3plugin.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::MockPlugins1
)

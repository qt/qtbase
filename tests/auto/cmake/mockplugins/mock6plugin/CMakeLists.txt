qt_internal_add_plugin(QMock6Plugin
    CLASS_NAME QMock6Plugin
    PLUGIN_TYPE mockauxplugin
    SOURCES
        qmock6plugin.cpp qmock6plugin.h
    LIBRARIES
        Qt::Core
        Qt::MockPlugins3
)

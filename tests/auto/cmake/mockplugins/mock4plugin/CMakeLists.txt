qt_internal_add_plugin(QMock4Plugin
    CLASS_NAME QMock4Plugin
    PLUGIN_TYPE mockplugin
    DEFAULT_IF FALSE
    SOURCES
        qmock4plugin.cpp qmock4plugin.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::MockPlugins1
)

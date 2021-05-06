qt_internal_add_plugin(QMock6Plugin
    CLASS_NAME QMock6Plugin
    TYPE mockauxplugin
    SOURCES
        qmock6plugin.cpp qmock6plugin.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::MockPlugins3
)

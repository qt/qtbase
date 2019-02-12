# Generated from vnc.pro.

#####################################################################
## qvnc Plugin:
#####################################################################

add_qt_plugin(qvnc
    TYPE platforms
    SOURCES
        main.cpp
        qvnc.cpp qvnc_p.h
        qvncclient.cpp qvncclient.h
        qvncintegration.cpp qvncintegration.h
        qvncscreen.cpp qvncscreen.h
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::GuiPrivate
        Qt::ServiceSupportPrivate
        Qt::ThemeSupportPrivate
        Qt::FbSupportPrivate
        Qt::EventDispatcherSupportPrivate
        Qt::FontDatabaseSupportPrivate
    # OTHER_FILES = "vnc.json"
    # PLUGIN_CLASS_NAME = "QVncIntegrationPlugin"
    # _LOADED = "qt_plugin"
)

## Scopes:
#####################################################################

extend_target(qvnc CONDITION TARGET Qt::InputSupportPrivate
    LIBRARIES
        Qt::InputSupportPrivate
)
#
#extend_target(qvnc CONDITION NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN
#    # PLUGIN_EXTENDS = "-"
#)

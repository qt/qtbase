# Generated from linuxfb.pro.

#####################################################################
## qlinuxfb Plugin:
#####################################################################

add_qt_plugin(qlinuxfb
    TYPE platforms
    SOURCES
        main.cpp
        qlinuxfbintegration.cpp qlinuxfbintegration.h
        qlinuxfbscreen.cpp qlinuxfbscreen.h
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::ServiceSupportPrivate
        Qt::EventDispatcherSupportPrivate
        Qt::FontDatabaseSupportPrivate
        Qt::FbSupportPrivate
    # OTHER_FILES = "linuxfb.json"
    # PLUGIN_CLASS_NAME = "QLinuxFbIntegrationPlugin"
    # _LOADED = "qt_plugin"
)

## Scopes:
#####################################################################

extend_target(qlinuxfb CONDITION TARGET Qt::InputSupportPrivate
    LIBRARIES
        Qt::InputSupportPrivate
)

extend_target(qlinuxfb CONDITION TARGET Qt::KmsSupportPrivate
    SOURCES
        qlinuxfbdrmscreen.cpp qlinuxfbdrmscreen.h
    LIBRARIES
        Qt::KmsSupportPrivate
)
#
#extend_target(qlinuxfb CONDITION NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN
#    # PLUGIN_EXTENDS = "-"
#)

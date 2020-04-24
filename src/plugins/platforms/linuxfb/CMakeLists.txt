# Generated from linuxfb.pro.

#####################################################################
## QLinuxFbIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QLinuxFbIntegrationPlugin
    OUTPUT_NAME qlinuxfb
    TYPE platforms
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES linuxfb # special case
    SOURCES
        main.cpp
        qlinuxfbintegration.cpp qlinuxfbintegration.h
        qlinuxfbscreen.cpp qlinuxfbscreen.h
    DEFINES
        QT_NO_FOREACH
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::EventDispatcherSupportPrivate
        Qt::FbSupportPrivate
        Qt::FontDatabaseSupportPrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::ServiceSupportPrivate
)

#### Keys ignored in scope 1:.:.:linuxfb.pro:<TRUE>:
# OTHER_FILES = "linuxfb.json"

## Scopes:
#####################################################################

qt_extend_target(QLinuxFbIntegrationPlugin CONDITION TARGET Qt::InputSupportPrivate
    PUBLIC_LIBRARIES
        Qt::InputSupportPrivate
)

qt_extend_target(QLinuxFbIntegrationPlugin CONDITION TARGET Qt::KmsSupportPrivate
    SOURCES
        qlinuxfbdrmscreen.cpp qlinuxfbdrmscreen.h
    PUBLIC_LIBRARIES
        Qt::KmsSupportPrivate
)

#### Keys ignored in scope 4:.:.:linuxfb.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"

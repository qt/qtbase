# Generated from linuxfb.pro.

#####################################################################
## QLinuxFbIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QLinuxFbIntegrationPlugin
    OUTPUT_NAME qlinuxfb
    PLUGIN_TYPE platforms
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES linuxfb # special case
    SOURCES
        main.cpp
        qlinuxfbintegration.cpp qlinuxfbintegration.h
        qlinuxfbscreen.cpp qlinuxfbscreen.h
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::FbSupportPrivate
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:linuxfb.pro:<TRUE>:
# OTHER_FILES = "linuxfb.json"

## Scopes:
#####################################################################

qt_internal_extend_target(QLinuxFbIntegrationPlugin CONDITION TARGET Qt::InputSupportPrivate
    LIBRARIES
        Qt::InputSupportPrivate
)

qt_internal_extend_target(QLinuxFbIntegrationPlugin CONDITION TARGET Qt::KmsSupportPrivate
    SOURCES
        qlinuxfbdrmscreen.cpp qlinuxfbdrmscreen.h
    LIBRARIES
        Qt::KmsSupportPrivate
)

#### Keys ignored in scope 4:.:.:linuxfb.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"

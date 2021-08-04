# Generated from vnc.pro.

#####################################################################
## QVncIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QVncIntegrationPlugin
    OUTPUT_NAME qvnc
    PLUGIN_TYPE platforms
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES vnc # special case
    SOURCES
        main.cpp
        qvnc.cpp qvnc_p.h
        qvncclient.cpp qvncclient.h
        qvncintegration.cpp qvncintegration.h
        qvncscreen.cpp qvncscreen.h
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::FbSupportPrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
)

#### Keys ignored in scope 1:.:.:vnc.pro:<TRUE>:
# OTHER_FILES = "vnc.json"

## Scopes:
#####################################################################

qt_internal_extend_target(QVncIntegrationPlugin CONDITION TARGET Qt::InputSupportPrivate
    LIBRARIES
        Qt::InputSupportPrivate
)

#### Keys ignored in scope 3:.:.:vnc.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"

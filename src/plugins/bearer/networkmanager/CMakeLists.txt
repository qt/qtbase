# Generated from networkmanager.pro.

#####################################################################
## QNetworkManagerEnginePlugin Plugin:
#####################################################################

qt_add_plugin(QNetworkManagerEnginePlugin
    OUTPUT_NAME qnmbearer
    TYPE bearer
    SOURCES
        main.cpp
        qnetworkmanagerengine.cpp qnetworkmanagerengine.h
        qnetworkmanagerservice.cpp qnetworkmanagerservice.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::DBus
        Qt::LinuxOfonoSupportPrivate
        Qt::NetworkPrivate
)

#### Keys ignored in scope 1:.:.:networkmanager.pro:<TRUE>:
# OTHER_FILES = "networkmanager.json"

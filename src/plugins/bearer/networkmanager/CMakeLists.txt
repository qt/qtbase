# Generated from networkmanager.pro.

#####################################################################
## qnmbearer Plugin:
#####################################################################

qt_add_plugin(qnmbearer
    TYPE bearer
    CLASS_NAME QNetworkManagerEnginePlugin
    SOURCES
        main.cpp
        qnetworkmanagerengine.cpp qnetworkmanagerengine.h
        qnetworkmanagerservice.cpp qnetworkmanagerservice.h
    LIBRARIES
        Qt::LinuxOfonoSupportPrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::DBus
        Qt::LinuxOfonoSupport
        Qt::Network
)

#### Keys ignored in scope 1:.:.:networkmanager.pro:<TRUE>:
# OTHER_FILES = "networkmanager.json"

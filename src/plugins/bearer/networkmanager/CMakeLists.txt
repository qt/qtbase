# Generated from networkmanager.pro.

#####################################################################
## qnmbearer Plugin:
#####################################################################

add_qt_plugin(qnmbearer
    TYPE bearer
    CLASS_NAME QNetworkManagerEnginePlugin
    SOURCES
        ../linux_common/qofonoservice_linux.cpp ../linux_common/qofonoservice_linux_p.h
        ../qbearerengine_impl.h
        ../qnetworksession_impl.cpp ../qnetworksession_impl.h
        main.cpp
        qnetworkmanagerengine.cpp qnetworkmanagerengine.h
        qnetworkmanagerservice.cpp qnetworkmanagerservice.h
    LIBRARIES
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::DBus
        Qt::Network
)

#### Keys ignored in scope 1:.:.:networkmanager.pro:<TRUE>:
# OTHER_FILES = "networkmanager.json"
# _LOADED = "qt_plugin"

# Generated from networkmanager.pro.

#####################################################################
## qnmbearer Plugin:
#####################################################################

qt_add_plugin(qnmbearer
    TYPE bearer
    DEFAULT_IF False # special case QTBUG-78164
    CLASS_NAME QNetworkManagerEnginePlugin
    SOURCES
        ../linux_common/qofonoservice_linux.cpp ../linux_common/qofonoservice_linux_p.h
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

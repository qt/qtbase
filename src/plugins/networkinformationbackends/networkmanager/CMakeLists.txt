qt_internal_add_plugin(QNetworkManagerNetworkInformationBackend
    OUTPUT_NAME networkmanagernetworkinformationbackend
    TYPE networkinformationbackends
    DEFAULT_IF LINUX
    SOURCES
        qnetworkmanagernetworkinformationbackend.cpp
        qnetworkmanagerservice.h
        qnetworkmanagerservice.cpp
    PUBLIC_LIBRARIES
        Qt::DBus
        Qt::NetworkPrivate
)

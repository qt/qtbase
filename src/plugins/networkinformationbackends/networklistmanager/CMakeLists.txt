qt_internal_add_plugin(QNetworkListManagerNetworkInformationBackend
    OUTPUT_NAME networklistmanagernetworkinformationbackend
    TYPE networkinformationbackends
    DEFAULT_IF WINDOWS AND QT_FEATURE_networklistmanager
    SOURCES qnetworklistmanagernetworkinformationbackend.cpp
    PUBLIC_LIBRARIES
        Qt::NetworkPrivate
)

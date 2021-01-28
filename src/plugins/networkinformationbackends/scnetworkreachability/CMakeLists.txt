qt_internal_add_plugin(QSCNetworkReachabilityNetworkInformationBackend
    OUTPUT_NAME scnetworkreachabilitynetworkinformationbackend
    TYPE networkinformationbackends
    DEFAULT_IF APPLE
    SOURCES
        qscnetworkreachabilitynetworkinformationbackend.mm
    PUBLIC_LIBRARIES
        Qt::NetworkPrivate
        ${FWSystemConfiguration}
)

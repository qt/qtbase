qt_internal_add_plugin(QSCNetworkReachabilityNetworkInformationPlugin
    OUTPUT_NAME scnetworkreachability
    CLASS_NAME QSCNetworkReachabilityNetworkInformationBackendFactory
    TYPE networkinformation
    DEFAULT_IF APPLE
    SOURCES
        qscnetworkreachabilitynetworkinformationbackend.mm
    PUBLIC_LIBRARIES
        Qt::NetworkPrivate
        ${FWSystemConfiguration}
)

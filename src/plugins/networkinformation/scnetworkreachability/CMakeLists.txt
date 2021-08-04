qt_internal_add_plugin(QSCNetworkReachabilityNetworkInformationPlugin
    OUTPUT_NAME scnetworkreachability
    CLASS_NAME QSCNetworkReachabilityNetworkInformationBackendFactory
    PLUGIN_TYPE networkinformation
    DEFAULT_IF APPLE
    SOURCES
        qscnetworkreachabilitynetworkinformationbackend.mm
    LIBRARIES
        Qt::NetworkPrivate
        ${FWSystemConfiguration}
)

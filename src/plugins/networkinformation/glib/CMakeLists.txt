qt_internal_add_plugin(QGlibNetworkInformationPlugin
    OUTPUT_NAME qglib
    CLASS_NAME QGlibNetworkInformationBackendFactory
    PLUGIN_TYPE networkinformation
    DEFAULT_IF LINUX
    SOURCES
        qglibnetworkinformationbackend.cpp
    LIBRARIES
        Qt::NetworkPrivate
        GLIB2::GOBJECT
        GLIB2::GIO
    DEFINES
        QT_NO_SIGNALS_SLOTS_KEYWORDS
)

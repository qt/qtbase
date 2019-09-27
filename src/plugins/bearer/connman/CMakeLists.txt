# Generated from connman.pro.

#####################################################################
## qconnmanbearer Plugin:
#####################################################################

add_qt_plugin(qconnmanbearer
    TYPE bearer
    CLASS_NAME QConnmanEnginePlugin
    SOURCES
        ../linux_common/qofonoservice_linux.cpp ../linux_common/qofonoservice_linux_p.h
        ../qbearerengine_impl.h
        ../qnetworksession_impl.cpp ../qnetworksession_impl.h
        main.cpp
        qconnmanengine.cpp qconnmanengine.h
        qconnmanservice_linux.cpp qconnmanservice_linux_p.h
    LIBRARIES
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::DBus
        Qt::Network
)

#### Keys ignored in scope 1:.:.:connman.pro:<TRUE>:
# OTHER_FILES = "connman.json"
# PLUGIN_CLASS_NAME = "QConnmanEnginePlugin"
# _LOADED = "qt_plugin"

# Generated from generic.pro.

#####################################################################
## qgenericbearer Plugin:
#####################################################################

add_qt_plugin(qgenericbearer
    TYPE bearer
    CLASS_NAME QGenericEnginePlugin
    SOURCES
        ../platformdefs_win.h
        ../qbearerengine_impl.h
        ../qnetworksession_impl.cpp ../qnetworksession_impl.h
        main.cpp
        qgenericengine.cpp qgenericengine.h
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Network
)

#### Keys ignored in scope 1:.:.:generic.pro:<TRUE>:
# OTHER_FILES = "generic.json"
# PLUGIN_CLASS_NAME = "QGenericEnginePlugin"
# _LOADED = "qt_plugin"

## Scopes:
#####################################################################

extend_target(qgenericbearer CONDITION WIN32 AND NOT WINRT
    PUBLIC_LIBRARIES
        iphlpapi
)

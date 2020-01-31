# Generated from generic.pro.

#####################################################################
## QGenericEnginePlugin Plugin:
#####################################################################

qt_add_plugin(QGenericEnginePlugin
    OUTPUT_NAME qgenericbearer
    TYPE bearer
    SOURCES
        ../platformdefs_win.h
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

## Scopes:
#####################################################################

qt_extend_target(QGenericEnginePlugin CONDITION WIN32 AND NOT WINRT
    PUBLIC_LIBRARIES
        iphlpapi
)

# Generated from corewlan.pro.

#####################################################################
## qcorewlanbearer Plugin:
#####################################################################

add_qt_plugin(qcorewlanbearer
    TYPE bearer
    CLASS_NAME QCoreWlanEnginePlugin
    SOURCES
        ../qbearerengine_impl.h
        ../qnetworksession_impl.cpp ../qnetworksession_impl.h
        main.cpp
        qcorewlanengine.h qcorewlanengine.mm
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        ${FWFoundation}
        ${FWSystemConfiguration}
        Qt::Core
        Qt::Network
)

#### Keys ignored in scope 1:.:.:corewlan.pro:<TRUE>:
# OTHER_FILES = "corewlan.json"
# _LOADED = "qt_plugin"

## Scopes:
#####################################################################

extend_target(qcorewlanbearer CONDITION QT_FEATURE_corewlan
    PUBLIC_LIBRARIES
        ${FWCoreWLAN}
        ${FWSecurity}
)

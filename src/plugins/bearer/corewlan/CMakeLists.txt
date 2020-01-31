# Generated from corewlan.pro.

#####################################################################
## QCoreWlanEnginePlugin Plugin:
#####################################################################

qt_add_plugin(QCoreWlanEnginePlugin
    OUTPUT_NAME qcorewlanbearer
    TYPE bearer
    SOURCES
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

## Scopes:
#####################################################################

qt_extend_target(QCoreWlanEnginePlugin CONDITION QT_FEATURE_corewlan
    PUBLIC_LIBRARIES
        ${FWCoreWLAN}
        ${FWSecurity}
)

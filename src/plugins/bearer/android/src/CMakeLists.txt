# Generated from src.pro.

#####################################################################
## qandroidbearer Plugin:
#####################################################################

add_qt_plugin(qandroidbearer
    TYPE bearer
    CLASS_NAME QAndroidBearerEnginePlugin
    SOURCES
        ../../qbearerengine_impl.h
        ../../qnetworksession_impl.cpp ../../qnetworksession_impl.h
        main.cpp
        qandroidbearerengine.cpp qandroidbearerengine.h
        wrappers/androidconnectivitymanager.cpp wrappers/androidconnectivitymanager.h
    INCLUDE_DIRECTORIES
        wrappers
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Network
)

#### Keys ignored in scope 1:.:.:src.pro:<TRUE>:
# _LOADED = "qt_plugin"

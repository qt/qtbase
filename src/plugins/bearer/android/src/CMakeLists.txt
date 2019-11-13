# Generated from src.pro.

#####################################################################
## qandroidbearer Plugin:
#####################################################################

qt_add_plugin(qandroidbearer
    TYPE bearer
    CLASS_NAME QAndroidBearerEnginePlugin
    SOURCES
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

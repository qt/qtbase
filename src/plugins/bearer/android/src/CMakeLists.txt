# Generated from src.pro.

#####################################################################
## QAndroidBearerEnginePlugin Plugin:
#####################################################################

qt_add_plugin(QAndroidBearerEnginePlugin
    OUTPUT_NAME qandroidbearer
    TYPE bearer
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

# Generated from connman.pro.

#####################################################################
## qconnmanbearer Plugin:
#####################################################################

qt_add_plugin(qconnmanbearer
    TYPE bearer
    CLASS_NAME QConnmanEnginePlugin
    SOURCES
        main.cpp
        qconnmanengine.cpp qconnmanengine.h
        qconnmanservice_linux.cpp qconnmanservice_linux_p.h
    LIBRARIES
        Qt::LinuxOfonoSupportPrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::DBus
        Qt::LinuxOfonoSupport
        Qt::Network
)

#### Keys ignored in scope 1:.:.:connman.pro:<TRUE>:
# OTHER_FILES = "connman.json"

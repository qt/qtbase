# Generated from connman.pro.

#####################################################################
## QConnmanEnginePlugin Plugin:
#####################################################################

qt_add_plugin(QConnmanEnginePlugin
    OUTPUT_NAME qconnmanbearer
    TYPE bearer
    SOURCES
        main.cpp
        qconnmanengine.cpp qconnmanengine.h
        qconnmanservice_linux.cpp qconnmanservice_linux_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::DBus
        Qt::LinuxOfonoSupportPrivate
        Qt::NetworkPrivate
)

#### Keys ignored in scope 1:.:.:connman.pro:<TRUE>:
# OTHER_FILES = "connman.json"

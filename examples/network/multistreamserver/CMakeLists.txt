# Generated from multistreamserver.pro.

#####################################################################
## multistreamserver Binary:
#####################################################################

add_qt_executable(multistreamserver
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/multistreamserver"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/multistreamserver"
    SOURCES
        chatprovider.cpp chatprovider.h
        main.cpp
        movieprovider.cpp movieprovider.h
        provider.h
        server.cpp server.h
        timeprovider.cpp timeprovider.h
    LIBRARIES
        Qt::Network
        Qt::Widgets
)

#### Keys ignored in scope 1:.:multistreamserver.pro:<NONE>:
# EXAMPLE_FILES = "animation.gif"

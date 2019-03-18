# Generated from fortuneserver.pro.

#####################################################################
## fortuneserver Binary:
#####################################################################

add_qt_executable(fortuneserver
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/fortuneserver"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/fortuneserver"
    SOURCES
        main.cpp
        server.cpp server.h
    LIBRARIES
        Qt::Network
        Qt::Widgets
)

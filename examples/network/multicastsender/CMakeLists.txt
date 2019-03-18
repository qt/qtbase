# Generated from multicastsender.pro.

#####################################################################
## multicastsender Binary:
#####################################################################

add_qt_executable(multicastsender
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/multicastsender"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/multicastsender"
    SOURCES
        main.cpp
        sender.cpp sender.h
    LIBRARIES
        Qt::Network
        Qt::Widgets
)

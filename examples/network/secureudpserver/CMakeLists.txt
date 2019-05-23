# Generated from secureudpserver.pro.

#####################################################################
## secureudpserver Binary:
#####################################################################

add_qt_executable(secureudpserver
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/secureudpserver"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/secureudpserver"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h mainwindow.ui
        nicselector.cpp nicselector.h nicselector.ui
        server.cpp server.h
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

#### Keys ignored in scope 1:.:.:secureudpserver.pro:<TRUE>:
# TEMPLATE = "app"

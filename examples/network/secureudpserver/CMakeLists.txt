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
    LIBRARIES
        Qt::Widgets
        Qt::Network
)

#### Keys ignored in scope 1:.:secureudpserver.pro:<NONE>:
# TEMPLATE = "app"

# Generated from secureudpclient.pro.

#####################################################################
## secureudpclient Binary:
#####################################################################

add_qt_executable(secureudpclient
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/secureudpclient"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/secureudpclient"
    SOURCES
        addressdialog.cpp addressdialog.h addressdialog.ui
        association.cpp association.h
        main.cpp
        mainwindow.cpp mainwindow.h mainwindow.ui
    LIBRARIES
        Qt::Widgets
        Qt::Network
)

#### Keys ignored in scope 1:.:secureudpclient.pro:<NONE>:
# TEMPLATE = "app"

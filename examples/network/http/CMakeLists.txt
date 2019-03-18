# Generated from http.pro.

#####################################################################
## http Binary:
#####################################################################

add_qt_executable(http
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/http"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/http"
    SOURCES
        authenticationdialog.ui
        httpwindow.cpp httpwindow.h
        main.cpp
    LIBRARIES
        Qt::Network
        Qt::Widgets
)

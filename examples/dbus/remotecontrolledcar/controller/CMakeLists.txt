# Generated from controller.pro.

#####################################################################
## controller Binary:
#####################################################################

add_qt_executable(controller
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/dbus/remotecontrolledcar/controller"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/dbus/remotecontrolledcar/controller"
    SOURCES
        controller.cpp controller.h controller.ui
        main.cpp
    DBUS_INTERFACE_SOURCES
        car.xml
    LIBRARIES
        Qt::DBus
        Qt::Widgets
)

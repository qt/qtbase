# Generated from car.pro.

#####################################################################
## car Binary:
#####################################################################

add_qt_executable(car
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/dbus/remotecontrolledcar/car"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/dbus/remotecontrolledcar/car"
    SOURCES
        car.cpp car.h
        main.cpp
    DBUS_ADAPTOR_SOURCES
        car.xml
    LIBRARIES
        Qt::DBus
        Qt::Widgets
)

QT += dbus widgets

DBUS_ADAPTORS += car.xml
HEADERS += car.h
SOURCES += car.cpp main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/dbus/remotecontrolledcar/car
INSTALLS += target

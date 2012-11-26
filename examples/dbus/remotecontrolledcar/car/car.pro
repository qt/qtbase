QT += dbus widgets

DBUS_ADAPTORS += car.xml
HEADERS += car.h
SOURCES += car.cpp main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/dbus/remotecontrolledcar/car
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.xml
sources.path = $$[QT_INSTALL_EXAMPLES]/dbus/remotecontrolledcar/car
INSTALLS += target sources

simulator: warning(This example does not work on Simulator platform)

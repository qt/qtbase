QT += widgets
CONFIG += qdbus

# DBUS_ADAPTORS += car.xml
HEADERS += car.h car_adaptor.h
SOURCES += car.cpp main.cpp car_adaptor.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/remotecontrolledcar/car
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.xml
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/remotecontrolledcar/car
INSTALLS += target sources

simulator: warning(This example does not work on Simulator platform)

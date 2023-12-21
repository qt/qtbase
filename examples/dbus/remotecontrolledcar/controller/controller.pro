QT += dbus widgets

DBUS_INTERFACES += ../common/car.xml
HEADERS += controller.h
SOURCES += main.cpp controller.cpp
RESOURCES += icons.qrc

# Work-around CI issue. Not needed in user code.
CONFIG += no_batch

# install
target.path = $$[QT_INSTALL_EXAMPLES]/dbus/remotecontrolledcar/controller
INSTALLS += target

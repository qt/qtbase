QT += dbus widgets

DBUS_INTERFACES += ../common/car.xml
FORMS += controller.ui
HEADERS += controller.h
SOURCES += main.cpp controller.cpp

# Work-around CI issue. Not needed in user code.
CONFIG += no_batch

# install
target.path = $$[QT_INSTALL_EXAMPLES]/dbus/remotecontrolledcar/controller
INSTALLS += target

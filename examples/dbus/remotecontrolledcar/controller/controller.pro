QT += widgets
CONFIG += qdbus

# DBUS_INTERFACES += car.xml
FORMS += controller.ui
HEADERS += car_interface.h controller.h
SOURCES += main.cpp car_interface.cpp controller.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/remotecontrolledcar/controller
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro *.xml
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/remotecontrolledcar/controller
INSTALLS += target sources

simulator: warning(This example does not work on Simulator platform)

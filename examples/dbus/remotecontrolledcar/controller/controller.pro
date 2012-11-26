QT += dbus widgets

DBUS_INTERFACES += car.xml
FORMS += controller.ui
HEADERS += controller.h
SOURCES += main.cpp controller.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/dbus/remotecontrolledcar/controller
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro *.xml
sources.path = $$[QT_INSTALL_EXAMPLES]/dbus/remotecontrolledcar/controller
INSTALLS += target sources

simulator: warning(This example does not work on Simulator platform)

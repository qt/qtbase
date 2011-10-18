TEMPLATE = subdirs
SUBDIRS = car \
	  controller

# install
sources.files = *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/remotecontrolledcar
INSTALLS += sources

QT += widgets

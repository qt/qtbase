TEMPLATE = subdirs
SUBDIRS = car \
	  controller

# install
sources.files = *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus/remotecontrolledcar
INSTALLS += sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)

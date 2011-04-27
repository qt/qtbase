TEMPLATE = subdirs
SUBDIRS = listnames \
	  pingpong \
	  complexpingpong

!contains(QT_CONFIG, no-gui) {
    SUBDIRS += dbus-chat \
               remotecontrolledcar
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus
sources.files = *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dbus
INSTALLS += sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)

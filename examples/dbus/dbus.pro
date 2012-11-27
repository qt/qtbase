TEMPLATE = subdirs
SUBDIRS = listnames \
	  pingpong \
	  complexpingpong

!contains(QT_CONFIG, no-widgets) {
    SUBDIRS += chat \
               remotecontrolledcar
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/dbus
sources.files = *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/dbus
INSTALLS += sources

QT += widgets

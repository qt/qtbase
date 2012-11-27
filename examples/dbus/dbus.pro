TEMPLATE = subdirs
SUBDIRS = listnames \
	  pingpong \
	  complexpingpong

!contains(QT_CONFIG, no-widgets) {
    SUBDIRS += chat \
               remotecontrolledcar
}

QT += widgets

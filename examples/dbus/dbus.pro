TEMPLATE = subdirs
SUBDIRS = listnames \
	  pingpong \
	  complexpingpong

qtHaveModule(widgets) {
    SUBDIRS += chat \
               remotecontrolledcar
}

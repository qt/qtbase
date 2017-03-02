requires(qtHaveModule(dbus))

TEMPLATE = subdirs
SUBDIRS = listnames \
    pingpong

qtConfig(process): SUBDIRS += complexpingpong

qtHaveModule(widgets) {
    SUBDIRS += chat \
               remotecontrolledcar
}

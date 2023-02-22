requires(qtHaveModule(dbus))

TEMPLATE = subdirs
SUBDIRS = pingpong

qtConfig(process): SUBDIRS += complexpingpong

qtHaveModule(widgets) {
    SUBDIRS += chat \
               remotecontrolledcar
}

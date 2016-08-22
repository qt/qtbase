requires(qtHaveModule(dbus))

TEMPLATE = subdirs
SUBDIRS = listnames \
    pingpong

!uikit: SUBDIRS += complexpingpong

qtHaveModule(widgets) {
    SUBDIRS += chat \
               remotecontrolledcar
}

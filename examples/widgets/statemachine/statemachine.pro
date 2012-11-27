TEMPLATE      = subdirs
SUBDIRS       = \
                factorial \
                pingpong

!contains(QT_CONFIG, no-widgets) {
    SUBDIRS +=  \
                eventtransitions \
                rogue \
                trafficlight \
                twowaybutton
}

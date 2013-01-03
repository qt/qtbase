TEMPLATE      = subdirs
SUBDIRS       = \
                factorial \
                pingpong

qtHaveModule(widgets) {
    SUBDIRS +=  \
                eventtransitions \
                rogue \
                trafficlight \
                twowaybutton
}

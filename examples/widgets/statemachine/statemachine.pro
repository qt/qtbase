TEMPLATE      = subdirs
SUBDIRS       = \

!emscripten: SUBDIRS += \
                factorial \
                pingpong

qtHaveModule(widgets) {
    SUBDIRS +=  \
                eventtransitions \
                rogue \
                trafficlight \
                twowaybutton
}

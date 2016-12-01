TEMPLATE      = subdirs
SUBDIRS       = basiclayouts \
                borderlayout \
                dynamiclayouts

!emscripten: SUBDIRS += \
                flowlayout

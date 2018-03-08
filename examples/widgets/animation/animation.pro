TEMPLATE         = \
                 subdirs
SUBDIRS          += \
                 easing \
                 moveblocks \
                 states \
                 stickman

!emscripten: SUBDIRS += \
                 animatedtiles \
                 sub-attaq

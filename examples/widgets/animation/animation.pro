TEMPLATE         = \
                 subdirs
SUBDIRS          += \
                 appchooser \
                 easing \
                 moveblocks \
                 states \
                 stickman

!emscripten: SUBDIRS += \
                 animatedtiles \
                 sub-attaq

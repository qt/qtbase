include(xkbcommon.pri)

# Build xkbcommon-x11 support library, it depends on -lxcb and -lxcb-xkb, linking is done
# in xcb-plugin.pro (linked to system libraries or if Qt was configured with -qt-xcb then
# linked to -lxcb-static).
INCLUDEPATH += $$PWD/xkbcommon/src/x11
SOURCES += \
    $$PWD/xkbcommon/src/x11/util.c \
    $$PWD/xkbcommon/src/x11/x11-keymap.c \ # renamed: keymap.c -> x11-keymap.c
    $$PWD/xkbcommon/src/x11/x11-state.c    # renamed: state.c  -> x11-state.c

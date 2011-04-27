INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/wayland-xcomposite-client-protocol.h \
    gl_integration/xcomposite_share/qwaylandxcompositebuffer.h

SOURCES += \
   $$PWD/wayland-xcomposite-protocol.c \
    gl_integration/xcomposite_share/qwaylandxcompositebuffer.cpp

DEFINES += QT_WAYLAND_WINDOWMANAGER_SUPPORT

contains(DEFINES, QT_WAYLAND_WINDOWMANAGER_SUPPORT) {

    HEADERS += \
        $$PWD/qwaylandwindowmanager-client-protocol.h \
        $$PWD/qwaylandwindowmanagerintegration.h

    SOURCES += \
        $$PWD/qwaylandwindowmanagerintegration.cpp \
        $$PWD/wayland-windowmanager-protocol.c

}




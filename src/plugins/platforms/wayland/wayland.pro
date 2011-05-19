TARGET = qwayland
include(../../qpluginbase.pri)

QT+=gui-private core-private opengl-private

DESTDIR = $$QT.gui.plugins/platforms

DEFINES += Q_PLATFORM_WAYLAND
DEFINES += $$QMAKE_DEFINES_WAYLAND

QT += core-private gui-private opengl-private

SOURCES =   main.cpp \
            qwaylandintegration.cpp \
            qwaylandnativeinterface.cpp \
            qwaylandshmsurface.cpp \
            qwaylandinputdevice.cpp \
            qwaylandcursor.cpp \
            qwaylanddisplay.cpp \
            qwaylandwindow.cpp \
            qwaylandscreen.cpp \
            qwaylandshmwindow.cpp \
            qwaylandclipboard.cpp

HEADERS =   qwaylandintegration.h \
            qwaylandnativeinterface.h \
            qwaylandcursor.h \
            qwaylanddisplay.h \
            qwaylandwindow.h \
            qwaylandscreen.h \
            qwaylandshmsurface.h \
            qwaylandbuffer.h \
            qwaylandshmwindow.h \
            qwaylandclipboard.h

INCLUDEPATH += $$QMAKE_INCDIR_WAYLAND
LIBS += $$QMAKE_LIBS_WAYLAND
QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_WAYLAND

INCLUDEPATH += $$PWD

include ($$PWD/gl_integration/gl_integration.pri)
include ($$PWD/windowmanager_integration/windowmanager_integration.pri)

include (../fontdatabases/genericunix/genericunix.pri)

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target


TARGET = qeglfs
load(qt_plugin)

QT += core-private gui-private platformsupport-private

DESTDIR = $$QT.gui.plugins/platforms

#DEFINES += QEGL_EXTRA_DEBUG

#DEFINES += Q_OPENKODE

#Avoid X11 header collision
DEFINES += MESA_EGL_NO_X11_HEADERS

#To test the hooks on x11 (xlib), comment the above define too
#EGLFS_PLATFORM_HOOKS_SOURCES += qeglfshooks_x11.cpp
#LIBS += -lX11

SOURCES =   main.cpp \
            qeglfsintegration.cpp \
            qeglfswindow.cpp \
            qeglfsbackingstore.cpp \
            qeglfsscreen.cpp \
            qeglfshooks_stub.cpp

HEADERS =   qeglfsintegration.h \
            qeglfswindow.h \
            qeglfsbackingstore.h \
            qeglfsscreen.h \
            qeglfshooks.h

QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

!isEmpty(EGLFS_PLATFORM_HOOKS_SOURCES) {
    HEADERS += $$EGLFS_PLATFORM_HOOKS_HEADERS
    SOURCES += $$EGLFS_PLATFORM_HOOKS_SOURCES
    DEFINES += EGLFS_PLATFORM_HOOKS
}

CONFIG += egl qpa/genericunixfontdatabase

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

OTHER_FILES += \
    eglfs.json

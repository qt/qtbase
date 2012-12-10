TARGET = qeglfs

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QEglFSIntegrationPlugin
load(qt_plugin)

QT += core-private gui-private platformsupport-private

#DEFINES += QEGL_EXTRA_DEBUG

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
            qeglfshooks_stub.cpp \
            qeglfscursor.cpp \
            qeglfscontext.cpp

HEADERS =   qeglfsintegration.h \
            qeglfswindow.h \
            qeglfsbackingstore.h \
            qeglfsscreen.h \
            qeglfscursor.h \
            qeglfshooks.h \
            qeglfscontext.h

QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

INCLUDEPATH += $$PWD

!isEmpty(EGLFS_PLATFORM_HOOKS_SOURCES) {
    HEADERS += $$EGLFS_PLATFORM_HOOKS_HEADERS
    SOURCES += $$EGLFS_PLATFORM_HOOKS_SOURCES
    LIBS    += $$EGLFS_PLATFORM_HOOKS_LIBS
    DEFINES += EGLFS_PLATFORM_HOOKS
}

CONFIG += egl qpa/genericunixfontdatabase

RESOURCES += cursor.qrc

OTHER_FILES += \
    eglfs.json

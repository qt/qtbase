# The device integration plugin base class has to live in a shared library,
# placing it into a static lib like platformsupport is not sufficient since we
# have to keep the QObject magic like qobject_cast working.
# Hence this header-less, private-only module.

TARGET = QtEglDeviceIntegration
CONFIG += no_module_headers internal_module

QT += core-private gui-private platformsupport-private
LIBS += $$QMAKE_LIBS_DYNLOAD

# Avoid X11 header collision
DEFINES += MESA_EGL_NO_X11_HEADERS

DEFINES += QT_BUILD_EGL_DEVICE_LIB

SOURCES +=  $$PWD/qeglfsintegration.cpp \
            $$PWD/qeglfswindow.cpp \
            $$PWD/qeglfsscreen.cpp \
            $$PWD/qeglfscursor.cpp \
            $$PWD/qeglfshooks.cpp \
            $$PWD/qeglfscontext.cpp \
            $$PWD/qeglfsoffscreenwindow.cpp \
            $$PWD/qeglfsdeviceintegration.cpp

HEADERS +=  $$PWD/qeglfsintegration.h \
            $$PWD/qeglfswindow.h \
            $$PWD/qeglfsscreen.h \
            $$PWD/qeglfscursor.h \
            $$PWD/qeglfshooks.h \
            $$PWD/qeglfscontext.h \
            $$PWD/qeglfsoffscreenwindow.h \
            $$PWD/qeglfsdeviceintegration.h

QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

INCLUDEPATH += $$PWD

!isEmpty(EGLFS_PLATFORM_HOOKS_SOURCES) {
    HEADERS += $$EGLFS_PLATFORM_HOOKS_HEADERS
    SOURCES += $$EGLFS_PLATFORM_HOOKS_SOURCES
    LIBS    += $$EGLFS_PLATFORM_HOOKS_LIBS
    DEFINES += EGLFS_PLATFORM_HOOKS
}

!isEmpty(EGLFS_DEVICE_INTEGRATION) {
    DEFINES += EGLFS_PREFERRED_PLUGIN=$$EGLFS_DEVICE_INTEGRATION
}

CONFIG += egl qpa/genericunixfontdatabase

!contains(DEFINES, QT_NO_CURSOR): RESOURCES += $$PWD/cursor.qrc

load(qt_module)

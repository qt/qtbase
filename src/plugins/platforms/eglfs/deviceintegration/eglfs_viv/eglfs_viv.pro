TARGET = qeglfs-viv-integration

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSVivIntegrationPlugin
load(qt_plugin)

QT += core-private gui-private platformsupport-private eglfs_device_lib-private

INCLUDEPATH += $$PWD/../..
CONFIG += egl
DEFINES += LINUX=1 EGL_API_FB=1
LIBS += -lGAL
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

SOURCES += $$PWD/qeglfsvivmain.cpp \
           $$PWD/qeglfsvivintegration.cpp

HEADERS += $$PWD/qeglfsvivintegration.h

OTHER_FILES += $$PWD/eglfs_viv.json

TARGET = qeglfs-brcm-integration

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSBrcmIntegrationPlugin
load(qt_plugin)

QT += core-private gui-private platformsupport-private eglfs_device_lib-private

INCLUDEPATH += $$PWD/../..
CONFIG += egl

LIBS += -lbcm_host
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

# Avoid X11 header collision
DEFINES += MESA_EGL_NO_X11_HEADERS

SOURCES += $$PWD/qeglfsbrcmmain.cpp \
           $$PWD/qeglfsbrcmintegration.cpp

HEADERS += $$PWD/qeglfsbrcmintegration.h

OTHER_FILES += $$PWD/eglfs_brcm.json

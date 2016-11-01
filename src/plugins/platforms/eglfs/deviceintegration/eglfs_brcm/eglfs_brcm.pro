TARGET = qeglfs-brcm-integration

QT += core-private gui-private eglfsdeviceintegration-private

INCLUDEPATH += $$PWD/../../api
CONFIG += egl

LIBS += -lbcm_host
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

SOURCES += $$PWD/qeglfsbrcmmain.cpp \
           $$PWD/qeglfsbrcmintegration.cpp

HEADERS += $$PWD/qeglfsbrcmintegration.h

OTHER_FILES += $$PWD/eglfs_brcm.json

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSBrcmIntegrationPlugin
load(qt_plugin)

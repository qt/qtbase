TARGET = qeglfs-emu-integration

QT += core-private gui-private eglfsdeviceintegration-private

INCLUDEPATH += $$PWD/../../api
CONFIG += egl

# Avoid X11 header collision
DEFINES += QT_EGL_NO_X11

OTHER_FILES += $$PWD/eglfs_emu.json

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSEmulatorIntegrationPlugin
load(qt_plugin)

DISTFILES += \
    eglfs_emu.json

SOURCES += \
    qeglfsemumain.cpp \
    qeglfsemulatorintegration.cpp \
    qeglfsemulatorscreen.cpp

HEADERS += \
    qeglfsemulatorintegration.h \
    qeglfsemulatorscreen.h

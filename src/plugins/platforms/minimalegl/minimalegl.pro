TARGET = qminimalegl

QT += \
    core-private gui-private \
    eventdispatcher_support-private fontdatabase_support-private egl_support-private

#DEFINES += QEGL_EXTRA_DEBUG

#DEFINES += Q_OPENKODE

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

SOURCES =   main.cpp \
            qminimaleglintegration.cpp \
            qminimaleglwindow.cpp \
            qminimaleglbackingstore.cpp \
            qminimaleglscreen.cpp

HEADERS =   qminimaleglintegration.h \
            qminimaleglwindow.h \
            qminimaleglbackingstore.h \
            qminimaleglscreen.h

CONFIG += egl

OTHER_FILES += \
    minimalegl.json

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QMinimalEglIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

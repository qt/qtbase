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
            qminimaleglscreen.cpp

HEADERS =   qminimaleglintegration.h \
            qminimaleglwindow.h \
            qminimaleglscreen.h

qtConfig(opengl) {
    SOURCES += qminimaleglbackingstore.cpp
    HEADERS += qminimaleglbackingstore.h
}

CONFIG += egl

OTHER_FILES += \
    minimalegl.json

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QMinimalEglIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

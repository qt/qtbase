TARGET = qxcb-egl-integration

PLUGIN_CLASS_NAME = QXcbEglIntegrationPlugin
PLUGIN_TYPE = xcbglintegrations

load(qt_plugin)

include(../gl_integrations_plugin_base.pri)

CONFIG += egl

DEFINES += SUPPORT_X11
#should be removed from sources
DEFINES += XCB_USE_EGL XCB_USE_XLIB


HEADERS += \
    qxcbeglcontext.h \
    qxcbeglintegration.h \
    qxcbeglwindow.h \
    qxcbeglnativeinterfacehandler.h

SOURCES += \
    qxcbeglintegration.cpp \
    qxcbeglwindow.cpp \
    qxcbeglmain.cpp \
    qxcbeglnativeinterfacehandler.cpp

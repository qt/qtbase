TARGET = qxcb-egl-integration

include(../gl_integrations_plugin_base.pri)
QT += egl_support-private

CONFIG += egl

qtConfig(xcb-xlib): DEFINES += XCB_USE_XLIB

DEFINES += QT_NO_FOREACH

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

PLUGIN_CLASS_NAME = QXcbEglIntegrationPlugin
PLUGIN_TYPE = xcbglintegrations
load(qt_plugin)

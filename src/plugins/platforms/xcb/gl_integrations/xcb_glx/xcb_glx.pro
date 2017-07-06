TARGET = qxcb-glx-integration

include(../gl_integrations_plugin_base.pri)
QT += glx_support-private

DEFINES += QT_NO_FOREACH

qtConfig(xcb-glx): QMAKE_USE += xcb_glx

!static:qtConfig(dlopen): QMAKE_USE += libdl

HEADERS += \
    qxcbglxintegration.h \
    qxcbglxwindow.h \
    qglxintegration.h \
    qxcbglxnativeinterfacehandler.h

SOURCES += \
    qxcbglxmain.cpp \
    qxcbglxintegration.cpp \
    qxcbglxwindow.cpp \
    qglxintegration.cpp \
    qxcbglxnativeinterfacehandler.cpp

PLUGIN_CLASS_NAME = QXcbGlxIntegrationPlugin
PLUGIN_TYPE = xcbglintegrations
load(qt_plugin)

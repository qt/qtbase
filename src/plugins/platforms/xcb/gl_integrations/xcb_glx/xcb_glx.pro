TARGET = qxcb-glx-integration

include(../gl_integrations_plugin_base.pri)
QT += glx_support-private

#should be removed from the sources
DEFINES += XCB_USE_GLX XCB_USE_XLIB
DEFINES += QT_NO_FOREACH

qtConfig(xcb-glx) {
    DEFINES += XCB_HAS_XCB_GLX
    QMAKE_USE += xcb_glx
}

LIBS += $$QMAKE_LIBS_DYNLOAD

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

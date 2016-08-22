TARGET = qxcb-glx-integration

include(../gl_integrations_plugin_base.pri)

#should be removed from the sources
DEFINES += XCB_USE_GLX XCB_USE_XLIB

qtConfig(xcb-glx) {
    DEFINES += XCB_HAS_XCB_GLX
    QMAKE_USE += xcb_glx
}
QMAKE_USE += xcb

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

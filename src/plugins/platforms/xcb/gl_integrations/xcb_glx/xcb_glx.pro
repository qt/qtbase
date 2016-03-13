TARGET = qxcb-glx-integration

include(../gl_integrations_plugin_base.pri)

#should be removed from the sources
DEFINES += XCB_USE_GLX XCB_USE_XLIB

LIBS += -lxcb

contains(QT_CONFIG, xcb-glx) {
    DEFINES += XCB_HAS_XCB_GLX
    LIBS += -lxcb-glx
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

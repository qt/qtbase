TARGET = qoffscreen

QT += \
    core-private gui-private \
    eventdispatcher_support-private fontdatabase_support-private

DEFINES += QT_NO_FOREACH

SOURCES =   main.cpp \
            qoffscreenintegration.cpp \
            qoffscreenwindow.cpp \
            qoffscreencommon.cpp

HEADERS =   qoffscreenintegration.h \
            qoffscreenwindow.h \
            qoffscreencommon.h

OTHER_FILES += offscreen.json

qtConfig(xlib):qtConfig(opengl):!qtConfig(opengles2) {
    SOURCES += qoffscreenintegration_x11.cpp
    HEADERS += qoffscreenintegration_x11.h
    QT += glx_support-private
}

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QOffscreenIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

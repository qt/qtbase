TARGET = qconnmanbearer

QT = core network-private dbus linuxofono_support_private

HEADERS += qconnmanservice_linux_p.h \
           qconnmanengine.h

SOURCES += main.cpp \
           qconnmanservice_linux.cpp \
           qconnmanengine.cpp

OTHER_FILES += connman.json

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = QConnmanEnginePlugin
load(qt_plugin)

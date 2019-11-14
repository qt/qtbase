TARGET = qnmbearer

QT = core network-private dbus linuxofono_support_private

HEADERS += qnetworkmanagerservice.h \
           qnetworkmanagerengine.h

SOURCES += main.cpp \
           qnetworkmanagerservice.cpp \
           qnetworkmanagerengine.cpp

OTHER_FILES += networkmanager.json

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = QNetworkManagerEnginePlugin
load(qt_plugin)

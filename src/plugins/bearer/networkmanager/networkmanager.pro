TARGET = qnmbearer

QT = core network-private dbus

HEADERS += qnetworkmanagerservice.h \
           qnetworkmanagerengine.h \
           ../linux_common/qofonoservice_linux_p.h

SOURCES += main.cpp \
           qnetworkmanagerservice.cpp \
           qnetworkmanagerengine.cpp \
           ../linux_common/qofonoservice_linux.cpp

OTHER_FILES += networkmanager.json

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = QNetworkManagerEnginePlugin
load(qt_plugin)

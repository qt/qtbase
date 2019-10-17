TARGET = qnativewifibearer

QT = core-private network-private

HEADERS += qnativewifiengine.h \
           platformdefs.h

SOURCES += main.cpp \
           qnativewifiengine.cpp

OTHER_FILES += nativewifi.json

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = QNativeWifiEnginePlugin
load(qt_plugin)

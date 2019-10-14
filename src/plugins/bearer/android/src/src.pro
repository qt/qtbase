TARGET = qandroidbearer

QT = core-private network-private

HEADERS += qandroidbearerengine.h

SOURCES += main.cpp \
           qandroidbearerengine.cpp

include(wrappers/wrappers.pri)

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = QAndroidBearerEnginePlugin
load(qt_plugin)

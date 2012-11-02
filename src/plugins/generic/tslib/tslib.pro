TARGET = qtslibplugin

PLUGIN_TYPE = generic
load(qt_plugin)

HEADERS	= qtslib.h

SOURCES	= main.cpp \
	qtslib.cpp

QT += gui-private

LIBS += -lts

OTHER_FILES += tslib.json

TARGET = qbsdmouseplugin

PLUGIN_TYPE = generic
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QBsdMousePlugin
load(qt_plugin)

QT += core-private gui-private

HEADERS = qbsdmouse.h
SOURCES = main.cpp \
         qbsdmouse.cpp

OTHER_FILES += \
    qbsdmouse.json


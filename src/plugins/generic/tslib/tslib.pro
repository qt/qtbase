TARGET = qtslibplugin

SOURCES = main.cpp

QT += gui-private platformsupport-private

QMAKE_USE += tslib

OTHER_FILES += tslib.json

PLUGIN_TYPE = generic
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QTsLibPlugin
load(qt_plugin)

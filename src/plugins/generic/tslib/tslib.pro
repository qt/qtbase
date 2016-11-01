TARGET = qtslibplugin

SOURCES = main.cpp

QT += core-private gui-private input_support-private

QMAKE_USE += tslib

OTHER_FILES += tslib.json

PLUGIN_TYPE = generic
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QTsLibPlugin
load(qt_plugin)

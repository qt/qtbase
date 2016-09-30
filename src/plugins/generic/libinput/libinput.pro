TARGET = qlibinputplugin

QT += core-private gui-private input_support-private

SOURCES = main.cpp

OTHER_FILES = libinput.json

PLUGIN_TYPE = generic
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QLibInputPlugin
load(qt_plugin)

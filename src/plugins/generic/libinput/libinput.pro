TARGET = qlibinputplugin

QT += core-private platformsupport-private gui-private

SOURCES = main.cpp

OTHER_FILES = libinput.json

PLUGIN_TYPE = generic
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QLibInputPlugin
load(qt_plugin)

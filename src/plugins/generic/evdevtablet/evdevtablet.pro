TARGET = qevdevtabletplugin

SOURCES = main.cpp

QT += core-private gui-private input_support-private

OTHER_FILES += \
    evdevtablet.json

PLUGIN_TYPE = generic
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QEvdevTabletPlugin
load(qt_plugin)

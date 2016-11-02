TARGET = qevdevmouseplugin

QT += core-private gui-private input_support-private

SOURCES = main.cpp

OTHER_FILES += \
    evdevmouse.json

PLUGIN_TYPE = generic
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QEvdevMousePlugin
load(qt_plugin)

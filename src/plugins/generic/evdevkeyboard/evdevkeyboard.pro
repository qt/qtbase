TARGET = qevdevkeyboardplugin

QT += core-private gui-private input_support-private

SOURCES = main.cpp

OTHER_FILES += \
    evdevkeyboard.json

PLUGIN_TYPE = generic
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QEvdevKeyboardPlugin
load(qt_plugin)

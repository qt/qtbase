TARGET = qevdevtouchplugin

SOURCES = main.cpp

QT += core-private platformsupport-private gui-private

OTHER_FILES += \
    evdevtouch.json

PLUGIN_TYPE = generic
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QEvdevTouchScreenPlugin
load(qt_plugin)

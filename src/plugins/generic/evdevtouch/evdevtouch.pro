TARGET = qevdevtouchplugin

PLUGIN_TYPE = generic
PLUGIN_CLASS_NAME = QEvdevTouchScreenPlugin
load(qt_plugin)

SOURCES = main.cpp

QT += core-private platformsupport-private gui-private

OTHER_FILES += \
    evdevtouch.json


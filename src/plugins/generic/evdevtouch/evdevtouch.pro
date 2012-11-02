TARGET = qevdevtouchplugin

PLUGIN_TYPE = generic
load(qt_plugin)

SOURCES = main.cpp

QT += core-private platformsupport-private gui-private

OTHER_FILES += \
    evdevtouch.json


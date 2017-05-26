TEMPLATE = lib
TARGET = moctestplugin
CONFIG += plugin static
SOURCES = main.cpp
plugin_resource.files = main.cpp
plugin_resource.prefix = /staticplugin
RESOURCES += plugin_resource
QT = core

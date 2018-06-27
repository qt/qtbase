TEMPLATE = lib
CONFIG += plugin static
SOURCES = main.cpp
QT = core

# Add extra metadata to the plugin
QMAKE_MOC_OPTIONS += -M ExtraMetaData=StaticPlugin -M ExtraMetaData=foo

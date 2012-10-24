TARGET = qlinuxfb

PLUGIN_TYPE = platforms
load(qt_plugin)

QT += core-private gui-private platformsupport-private

SOURCES = main.cpp qlinuxfbintegration.cpp qlinuxfbscreen.cpp
HEADERS = qlinuxfbintegration.h qlinuxfbscreen.h

CONFIG += qpa/genericunixfontdatabase

OTHER_FILES += linuxfb.json

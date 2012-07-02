TARGET = qlinuxfb
load(qt_plugin)

DESTDIR = $$QT.gui.plugins/platforms

QT += core-private gui-private platformsupport-private

SOURCES = main.cpp qlinuxfbintegration.cpp qlinuxfbscreen.cpp
HEADERS = qlinuxfbintegration.h qlinuxfbscreen.h

CONFIG += qpa/genericunixfontdatabase

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

OTHER_FILES += linuxfb.json

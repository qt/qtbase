TARGET = qlinuxfbgraphicssystem
load(qt_plugin)

DESTDIR = $$QT.gui.plugins/platforms

SOURCES = main.cpp qlinuxfbintegration.cpp
HEADERS = qlinuxfbintegration.h

include(../fb_base/fb_base.pri)
include(../fontdatabases/genericunix/genericunix.pri)

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

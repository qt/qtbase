TARGET = qlinuxfbgraphicssystem
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms

SOURCES = main.cpp qlinuxfbintegration.cpp
HEADERS = qlinuxfbintegration.h

include(../fb_base/fb_base.pri)
include(../fontdatabases/genericunix/genericunix.pri)

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

TARGET = qvfbintegration
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms


SOURCES = main.cpp qvfbintegration.cpp qvfbwindowsurface.cpp
HEADERS = qvfbintegration.h qvfbwindowsurface.h

include(../fontdatabases/genericunix/genericunix.pri)

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

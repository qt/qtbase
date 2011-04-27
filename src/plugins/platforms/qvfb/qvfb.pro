TARGET = qvfbintegration
include(../../qpluginbase.pri)

DESTDIR = $$QT.gui.plugins/platforms


SOURCES = main.cpp qvfbintegration.cpp qvfbwindowsurface.cpp
HEADERS = qvfbintegration.h qvfbwindowsurface.h

include(../fontdatabases/genericunix/genericunix.pri)

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

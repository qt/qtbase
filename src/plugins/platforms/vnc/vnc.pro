TARGET = qvncgraphicssystem
load(qt_plugin)

QT += network

DESTDIR = $$QT.gui.plugins/platforms

SOURCES = main.cpp qvncintegration.cpp
HEADERS = qvncintegration.h

HEADERS += qvncserver.h
SOURCES += qvncserver.cpp

HEADERS += qvnccursor.h
SOURCES += qvnccursor.cpp

include(../fb_base/fb_base.pri)
include(../fontdatabases/genericunix/genericunix.pri)

target.path += $$[QT_INSTALL_PLUGINS]/platforms

INSTALLS += target

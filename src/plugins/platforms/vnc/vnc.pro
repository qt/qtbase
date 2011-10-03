TARGET = qvncgraphicssystem
load(qt_plugin)

QT += network core-private gui-private platformsupport-private

DESTDIR = $$QT.gui.plugins/platforms

SOURCES = main.cpp qvncintegration.cpp
HEADERS = qvncintegration.h

HEADERS += qvncserver.h
SOURCES += qvncserver.cpp

HEADERS += qvnccursor.h
SOURCES += qvnccursor.cpp

include(../fb_base/fb_base.pri)
CONFIG += qpa/genericunixfontdatabase

target.path += $$[QT_INSTALL_PLUGINS]/platforms

INSTALLS += target

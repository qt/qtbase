TARGET = qlinuxinputplugin
load(qt_plugin)

DESTDIR = $$QT.gui.plugins/generic
target.path = $$[QT_INSTALL_PLUGINS]/generic
INSTALLS += target

HEADERS	= qlinuxinput.h

QT += core-private

SOURCES	= main.cpp \
	qlinuxinput.cpp


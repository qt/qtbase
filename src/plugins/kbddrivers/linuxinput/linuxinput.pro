TARGET = qlinuxinputkbddriver
load(qt_plugin)

DESTDIR = $$QT.gui.plugins/kbddrivers
target.path = $$[QT_INSTALL_PLUGINS]/kbddrivers
INSTALLS += target

DEFINES += QT_QWS_KBD_LINUXINPUT

HEADERS	= $$QT_SOURCE_TREE/src/gui/embedded/qkbdlinuxinput_qws.h

SOURCES	= main.cpp \
	$$QT_SOURCE_TREE/src/gui/embedded/qkbdlinuxinput_qws.cpp


TARGET = qlinuxinputplugin
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/generic
target.path = $$[QT_INSTALL_PLUGINS]/generic
INSTALLS += target

DEFINES += QT_QWS_KBD_LINUXINPUT

HEADERS	= qlinuxinput.h

SOURCES	= main.cpp \
	qlinuxinput.cpp

HEADERS	+= $$QT_SOURCE_TREE/src/gui/embedded/qkbd_qws.h \
	$$QT_SOURCE_TREE/src/gui/embedded/qkbd_qws_p.h

SOURCES	+= $$QT_SOURCE_TREE/src/gui/embedded/qkbd_qws.cpp

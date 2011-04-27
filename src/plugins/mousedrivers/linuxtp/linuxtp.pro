TARGET = qlinuxtpmousedriver
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/mousedrivers
target.path = $$[QT_INSTALL_PLUGINS]/mousedrivers
INSTALLS += target

DEFINES += QT_QWS_MOUSE_LINUXTP

HEADERS	= $$QT_SOURCE_TREE/src/gui/embedded/qmouselinuxtp_qws.h

SOURCES	= main.cpp \
	$$QT_SOURCE_TREE/src/gui/embedded/qmouselinuxtp_qws.cpp


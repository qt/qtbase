TARGET = qscreenlinuxfb
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/gfxdrivers

target.path = $$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS += target

DEFINES += QT_QWS_LINUXFB

HEADERS	= $$QT_SOURCE_TREE/src/gui/embedded/qscreenlinuxfb_qws.h

SOURCES	= main.cpp \
	$$QT_SOURCE_TREE/src/gui/embedded/qscreenlinuxfb_qws.cpp

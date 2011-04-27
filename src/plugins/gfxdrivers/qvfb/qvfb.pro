TARGET = qscreenvfb
include(../../qpluginbase.pri)

DEFINES	+= QT_QWS_QVFB QT_QWS_MOUSE_QVFB QT_QWS_KBD_QVFB

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/gfxdrivers

HEADERS = \
	$$QT_SOURCE_TREE/src/gui/embedded/qscreenvfb_qws.h \
	$$QT_SOURCE_TREE/src/gui/embedded/qkbdvfb_qws.h \
	$$QT_SOURCE_TREE/src/gui/embedded/qmousevfb_qws.h

SOURCES	= main.cpp \
	$$QT_SOURCE_TREE/src/gui/embedded/qscreenvfb_qws.cpp \
	$$QT_SOURCE_TREE/src/gui/embedded/qkbdvfb_qws.cpp \
	$$QT_SOURCE_TREE/src/gui/embedded/qmousevfb_qws.cpp

target.path += $$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS += target

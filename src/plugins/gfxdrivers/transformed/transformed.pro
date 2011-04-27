TARGET	 = qgfxtransformed
include(../../qpluginbase.pri)

DEFINES	+= QT_QWS_TRANSFORMED

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/gfxdrivers

HEADERS	= $$QT_SOURCE_TREE/src/gui/embedded/qscreentransformed_qws.h
SOURCES	= main.cpp \
	  $$QT_SOURCE_TREE/src/gui/embedded/qscreentransformed_qws.cpp

target.path=$$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS += target

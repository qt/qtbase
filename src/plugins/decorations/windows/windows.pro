TARGET	 = qdecorationwindows
include(../../qpluginbase.pri)

HEADERS	= $$QT_SOURCE_TREE/src/gui/embedded/qdecorationwindows_qws.h
SOURCES	= main.cpp \
	  $$QT_SOURCE_TREE/src/gui/embedded/qdecorationwindows_qws.cpp

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/decorations
target.path += $$[QT_INSTALL_PLUGINS]/decorations
INSTALLS += target

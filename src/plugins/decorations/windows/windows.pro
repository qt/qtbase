TARGET	 = qdecorationwindows
load(qt_plugin)

HEADERS	= $$QT_SOURCE_TREE/src/gui/embedded/qdecorationwindows_qws.h
SOURCES	= main.cpp \
	  $$QT_SOURCE_TREE/src/gui/embedded/qdecorationwindows_qws.cpp

DESTDIR = $$QT.gui.plugins/decorations
target.path += $$[QT_INSTALL_PLUGINS]/decorations
INSTALLS += target

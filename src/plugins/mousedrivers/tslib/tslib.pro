TARGET	 = qtslibmousedriver
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/mousedrivers

HEADERS = \
	$$QT_SOURCE_TREE/src/gui/embedded/qmousedriverplugin_qws.h \
	$$QT_SOURCE_TREE/src/gui/embedded/qmousetslib_qws.h
SOURCES	= main.cpp \
	$$QT_SOURCE_TREE/src/gui/embedded/qmousetslib_qws.cpp

LIBS += -lts

target.path += $$[QT_INSTALL_PLUGINS]/mousedrivers
INSTALLS += target


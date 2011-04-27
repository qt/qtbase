TARGET = qlinuxinputplugin
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/generic
target.path = $$[QT_INSTALL_PLUGINS]/generic
INSTALLS += target

HEADERS	= qtslib.h

SOURCES	= main.cpp \
	qtslib.cpp

LIBS += -lts

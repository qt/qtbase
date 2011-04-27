TARGET = qahiscreen
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/gfxdrivers

target.path = $$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS += target

HEADERS	= qscreenahi_qws.h

SOURCES	= qscreenahi_qws.cpp \
          qscreenahiplugin.cpp

LIBS += -lahi

TARGET = qahiscreen
include(../../qpluginbase.pri)

DESTDIR = $$QT.gui.plugins/gfxdrivers

target.path = $$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS += target

HEADERS	= qscreenahi_qws.h

SOURCES	= qscreenahi_qws.cpp \
          qscreenahiplugin.cpp

LIBS += -lahi

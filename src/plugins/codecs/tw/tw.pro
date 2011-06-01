TARGET   = qtwcodecs
load(qt_plugin)

CONFIG  += warn_on
DESTDIR = $$QT.core.plugins/codecs
QT = core

HEADERS  = qbig5codec.h

SOURCES  = qbig5codec.cpp \
	   main.cpp

target.path += $$[QT_INSTALL_PLUGINS]/codecs
INSTALLS += target

symbian:TARGET.UID3=0x2001B2E4

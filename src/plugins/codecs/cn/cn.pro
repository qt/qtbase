TARGET	 = qcncodecs
include(../../qpluginbase.pri)

CONFIG	+= warn_on
DESTDIR = $$QT.core.plugins/codecs
QT = core

HEADERS		= qgb18030codec.h

SOURCES		= qgb18030codec.cpp \
		  main.cpp

target.path += $$[QT_INSTALL_PLUGINS]/codecs
INSTALLS += target

symbian:TARGET.UID3=0x2001E615

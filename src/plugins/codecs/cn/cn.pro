TARGET	 = qcncodecs
load(qt_plugin)

CONFIG	+= warn_on
DESTDIR = $$QT.core.plugins/codecs
QT = core

HEADERS		= qgb18030codec.h

SOURCES		= qgb18030codec.cpp \
		  main.cpp

target.path += $$[QT_INSTALL_PLUGINS]/codecs
INSTALLS += target

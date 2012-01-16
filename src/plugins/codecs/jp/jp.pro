TARGET	 = qjpcodecs
load(qt_plugin)

CONFIG	+= warn_on
DESTDIR = $$QT.core.plugins/codecs
QT = core

HEADERS		= qjpunicode.h \
                  qeucjpcodec.h \
		  qjiscodec.h \
		  qsjiscodec.h 

SOURCES		= qeucjpcodec.cpp \
		  qjiscodec.cpp \
		  qsjiscodec.cpp \
		  qjpunicode.cpp \
		  main.cpp

unix {
	HEADERS += qfontjpcodec.h
	SOURCES += qfontjpcodec.cpp
}

target.path += $$[QT_INSTALL_PLUGINS]/codecs
INSTALLS += target

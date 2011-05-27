TARGET	 = qkrcodecs
load(qt_plugin)

CONFIG	+= warn_on
DESTDIR = $$QT.core.plugins/codecs
QT = core

HEADERS		= qeuckrcodec.h \
              cp949codetbl.h              
SOURCES		= qeuckrcodec.cpp \
		  main.cpp

wince*: {
   SOURCES += ../../../corelib/kernel/qfunctions_wince.cpp
}

target.path += $$[QT_INSTALL_PLUGINS]/codecs
INSTALLS += target

symbian:TARGET.UID3=0x2001B2E5

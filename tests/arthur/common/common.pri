VPATH+=$$PWD
INCLUDEPATH += $$PWD

contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles1)|contains(QT_CONFIG, opengles2):DEFINES += BUILD_OPENGL

SOURCES += \
	xmldata.cpp \
	paintcommands.cpp \
	qengines.cpp \
	framework.cpp

HEADERS += \
	xmldata.h \
	paintcommands.h \
	qengines.h \
	framework.h

RESOURCES += images.qrc

TEMPLATE = lib
CONFIG += static

qtHaveModule(opengl) {
	DEFINES += QT_OPENGL_SUPPORT
	QT += opengl
}
QT += widgets

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}
TARGET = demo_shared

SOURCES += \
	arthurstyle.cpp\
	arthurwidgets.cpp \
	hoverpoints.cpp

HEADERS += \
	arthurstyle.h \
	arthurwidgets.h \
	hoverpoints.h

RESOURCES += shared.qrc

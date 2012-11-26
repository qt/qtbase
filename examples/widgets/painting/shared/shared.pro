TEMPLATE = lib
CONFIG += static

contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles1)|contains(QT_CONFIG, opengles2) {
	DEFINES += QT_OPENGL_SUPPORT
	QT += opengl widgets
}

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}
TARGET = demo_shared
QT += widgets

SOURCES += \
	arthurstyle.cpp\
	arthurwidgets.cpp \
	hoverpoints.cpp

HEADERS += \
	arthurstyle.h \
	arthurwidgets.h \
	hoverpoints.h

RESOURCES += shared.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/shared
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.pri images
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/shared
INSTALLS += sources

!cross_compile:INSTALLS += target

SOURCES += main.cpp pathdeform.cpp
HEADERS += pathdeform.h

SHARED_FOLDER = ../shared

include($$SHARED_FOLDER/shared.pri)

RESOURCES += deform.qrc

contains(QT_CONFIG, opengl) {
	DEFINES += QT_OPENGL_SUPPORT
	QT += opengl widgets
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting/deform
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.html
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting/deform
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000A63D
    CONFIG += qt_example
}

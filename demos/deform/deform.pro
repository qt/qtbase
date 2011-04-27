SOURCES += main.cpp pathdeform.cpp
HEADERS += pathdeform.h

SHARED_FOLDER = ../shared

include($$SHARED_FOLDER/shared.pri)

RESOURCES += deform.qrc

contains(QT_CONFIG, opengl) {
	DEFINES += QT_OPENGL_SUPPORT
	QT += opengl
}

# install
target.path = $$[QT_INSTALL_DEMOS]/qtbase/deform
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.html
sources.path = $$[QT_INSTALL_DEMOS]/qtbase/deform
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000A63D
    CONFIG += qt_demo
}

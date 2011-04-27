SOURCES += main.cpp pathstroke.cpp
HEADERS += pathstroke.h

SHARED_FOLDER = ../shared

include($$SHARED_FOLDER/shared.pri)

RESOURCES += pathstroke.qrc

contains(QT_CONFIG, opengl) {
	DEFINES += QT_OPENGL_SUPPORT
	QT += opengl
}

# install
target.path = $$[QT_INSTALL_DEMOS]/qtbase/pathstroke
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.html
sources.path = $$[QT_INSTALL_DEMOS]/qtbase/pathstroke
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000A63E
    include($$QT_SOURCE_TREE/demos/symbianpkgrules.pri)
}

SOURCES += main.cpp pathdeform.cpp
HEADERS += pathdeform.h

SHARED_FOLDER = ../shared

include($$SHARED_FOLDER/shared.pri)

RESOURCES += deform.qrc

qtHaveModule(opengl) {
	DEFINES += QT_OPENGL_SUPPORT
	QT += opengl
}
QT += widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/deform
INSTALLS += target

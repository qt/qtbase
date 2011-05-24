HEADERS += \
	coloritem.h \
	robot.h

SOURCES += \
	coloritem.cpp \
	main.cpp \
	robot.cpp

RESOURCES += \
	robot.qrc


# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/dragdroprobot
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS dragdroprobot.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/dragdroprobot
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
maemo5: CONFIG += qt_example

simulator: warning(This example might not fully work on Simulator platform)

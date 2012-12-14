QT += widgets

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
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/dragdroprobot
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)

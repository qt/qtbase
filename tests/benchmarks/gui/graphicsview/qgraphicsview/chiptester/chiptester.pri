SOURCES += \
	chiptester/chiptester.cpp \
	chiptester/chip.cpp

HEADERS += \
	chiptester/chiptester.h \
	chiptester/chip.h

RESOURCES += \
        chiptester/images.qrc

QT += widgets
qtHaveModule(opengl): QT += opengl

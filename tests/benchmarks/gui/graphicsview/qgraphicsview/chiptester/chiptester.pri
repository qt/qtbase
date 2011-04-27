SOURCES += \
	chiptester/chiptester.cpp \
	chiptester/chip.cpp

HEADERS += \
	chiptester/chiptester.h \
	chiptester/chip.h

RESOURCES += \
        chiptester/images.qrc

contains(QT_CONFIG, opengl) QT += opengl

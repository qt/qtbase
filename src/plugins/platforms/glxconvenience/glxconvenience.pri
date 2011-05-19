INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/qglxconvenience.h

SOURCES += \
    $$PWD/qglxconvenience.cpp

CONFIG += xrender

xrender {
	LIBS += -lXrender
} else {
	DEFINES += QT_NO_XRENDER
}

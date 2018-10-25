INCLUDEPATH += $$PWD

qtConfig(opengl) {
    SOURCES += $$PWD/fbopaintdevice.cpp
    HEADERS += $$PWD/fbopaintdevice.h
}

SOURCES += \
    $$PWD/arthurstyle.cpp\
    $$PWD/arthurwidgets.cpp \
    $$PWD/hoverpoints.cpp

HEADERS += \
    $$PWD/arthurstyle.h \
    $$PWD/arthurwidgets.h \
    $$PWD/hoverpoints.h

RESOURCES += $$PWD/shared.qrc


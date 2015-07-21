INCLUDEPATH += $$PWD
SOURCES += \
    $$PWD/textdump.cpp \
    $$PWD/eventfilter.cpp \
    $$PWD/qwindowdump.cpp \
    $$PWD/debugproxystyle.cpp

HEADERS += \
    $$PWD/textdump.h \
    $$PWD/eventfilter.h \
    $$PWD/qwindowdump.h \
    $$PWD/nativewindowdump.h \
    $$PWD/debugproxystyle.h

win32 {
    SOURCES += $$PWD/nativewindowdump_win.cpp
    LIBS *= -luser32
} else {
    SOURCES += $$PWD/nativewindowdump.cpp
}

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += gui-private core-private
    contains(QT, widgets) {
        HEADERS += \
            $$PWD/qwidgetdump.h

        SOURCES += \
            $$PWD/qwidgetdump.cpp
    }
} else {
    HEADERS += \
        $$PWD/qwidgetdump.h

    SOURCES += \
        $$PWD/qwidgetdump.cpp
}

contains(QT, opengl) {
HEADERS += \
    $$PWD/glinfo.h

SOURCES += \
    $$PWD/glinfo.cpp
}

DEFINES += QT_DIAG_LIB

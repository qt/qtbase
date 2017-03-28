INCLUDEPATH += $$PWD
SOURCES += \
    $$PWD/textdump.cpp \
    $$PWD/eventfilter.cpp \
    $$PWD/qwindowdump.cpp

HEADERS += \
    $$PWD/textdump.h \
    $$PWD/eventfilter.h \
    $$PWD/qwindowdump.h \
    $$PWD/nativewindowdump.h

win32:!winrt:  {
    SOURCES += $$PWD/nativewindowdump_win.cpp
    LIBS *= -luser32
} else {
    SOURCES += $$PWD/nativewindowdump.cpp
}

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += gui-private core-private
    contains(QT, widgets) {
        HEADERS += \
            $$PWD/qwidgetdump.h \
            $$PWD/debugproxystyle.h \
            $$PWD/logwidget.h

        SOURCES += \
            $$PWD/qwidgetdump.cpp \
            $$PWD/debugproxystyle.cpp \
            $$PWD/logwidget.cpp
    }
} else {
    HEADERS += \
        $$PWD/qwidgetdump.h \
        $$PWD/debugproxystyle.h \
        $$PWD/logwidget.h

    SOURCES += \
        $$PWD/qwidgetdump.cpp \
        $$PWD/debugproxystyle.cpp \
        $$PWD/logwidget.cpp
}

contains(QT, opengl) {
HEADERS += \
    $$PWD/glinfo.h

SOURCES += \
    $$PWD/glinfo.cpp
}

DEFINES += QT_DIAG_LIB

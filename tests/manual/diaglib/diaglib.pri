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

win32:  {
    SOURCES += $$PWD/nativewindowdump_win.cpp
    LIBS += -luser32
} else {
    SOURCES += $$PWD/nativewindowdump.cpp
}

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
    QT += widgets-private
}

contains(QT, opengl) {
    HEADERS += \
        $$PWD/glinfo.h

    SOURCES += \
        $$PWD/glinfo.cpp

    QT += opengl openglwidgets
}

DEFINES += QT_DIAG_LIB

HEADERS += \
    $$PWD/qlibinputhandler_p.h \
    $$PWD/qlibinputpointer_p.h \
    $$PWD/qlibinputkeyboard_p.h \
    $$PWD/qlibinputtouch_p.h

SOURCES += \
    $$PWD/qlibinputhandler.cpp \
    $$PWD/qlibinputpointer.cpp \
    $$PWD/qlibinputkeyboard.cpp \
    $$PWD/qlibinputtouch.cpp

INCLUDEPATH += $$QMAKE_INCDIR_LIBUDEV $$QMAKE_INCDIR_LIBINPUT
LIBS_PRIVATE += $$QMAKE_LIBS_LIBUDEV $$QMAKE_LIBS_LIBINPUT

contains(QT_CONFIG, xkbcommon-evdev) {
    INCLUDEPATH += $$QMAKE_INCDIR_XKBCOMMON_EVDEV
    LIBS_PRIVATE += $$QMAKE_LIBS_XKBCOMMON_EVDEV
} else {
    DEFINES += QT_NO_XKBCOMMON_EVDEV
}

DEFINES += QT_LIBINPUT_VERSION_MAJOR=$$QMAKE_LIBINPUT_VERSION_MAJOR QT_LIBINPUT_VERSION_MINOR=$$QMAKE_LIBINPUT_VERSION_MINOR

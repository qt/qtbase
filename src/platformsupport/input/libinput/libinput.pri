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

QMAKE_USE_PRIVATE += libudev libinput

INCLUDEPATH += $$PWD/../shared

contains(QT_CONFIG, xkbcommon-evdev) {
    QMAKE_USE_PRIVATE += xkbcommon_evdev
} else {
    DEFINES += QT_NO_XKBCOMMON_EVDEV
}

DEFINES += QT_LIBINPUT_VERSION_MAJOR=$$QMAKE_LIBINPUT_VERSION_MAJOR QT_LIBINPUT_VERSION_MINOR=$$QMAKE_LIBINPUT_VERSION_MINOR

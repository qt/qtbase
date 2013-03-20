linux:contains(QT_CONFIG, evdev) {
    HEADERS += $$PWD/qdevicediscovery_p.h

    contains(QT_CONFIG, libudev) {
        SOURCES += $$PWD/qdevicediscovery_udev.cpp

        INCLUDEPATH += $$QMAKE_INCDIR_LIBUDEV
        LIBS += $$QMAKE_LIBS_LIBUDEV
    } else {
        SOURCES += $$PWD/qdevicediscovery_static.cpp
    }
}

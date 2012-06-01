HEADERS += \
    $$PWD/qevdevtablet_p.h

SOURCES += \
    $$PWD/qevdevtablet.cpp

contains(QT_CONFIG, libudev) {
    LIBS += $$QMAKE_LIBS_LIBUDEV
}

HEADERS += \
    $$PWD/qevdevtouchhandler_p.h \
    $$PWD/qevdevtouchmanager_p.h

SOURCES += \
    $$PWD/qevdevtouchhandler.cpp \
    $$PWD/qevdevtouchmanager.cpp

INCLUDEPATH += $$PWD/../shared

qtConfig(libudev): \
    QMAKE_USE_PRIVATE += libudev

qtConfig(mtdev) {
    QMAKE_USE_PRIVATE += mtdev
}

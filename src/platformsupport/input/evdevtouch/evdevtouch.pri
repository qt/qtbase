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
    CONFIG += link_pkgconfig
    PKGCONFIG_PRIVATE += mtdev
}

HEADERS += \
    $$PWD/qevdevtouchhandler_p.h \
    $$PWD/qevdevtouchmanager_p.h

SOURCES += \
    $$PWD/qevdevtouchhandler.cpp \
    $$PWD/qevdevtouchmanager.cpp

INCLUDEPATH += $$PWD/../shared

contains(QT_CONFIG, libudev) {
    QMAKE_USE_PRIVATE += libudev
}

contains(QT_CONFIG, mtdev) {
    CONFIG += link_pkgconfig
    PKGCONFIG_PRIVATE += mtdev
}

HEADERS += \
    $$PWD/qevdevtouchhandler_p.h \
    $$PWD/qevdevtouchmanager_p.h

SOURCES += \
    $$PWD/qevdevtouchhandler.cpp \
    $$PWD/qevdevtouchmanager.cpp

contains(QT_CONFIG, libudev) {
    LIBS_PRIVATE += $$QMAKE_LIBS_LIBUDEV
}

contains(QT_CONFIG, mtdev) {
    CONFIG += link_pkgconfig
    PKGCONFIG_PRIVATE += mtdev
}


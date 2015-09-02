HEADERS += \
    $$PWD/qevdevtablethandler_p.h \
    $$PWD/qevdevtabletmanager_p.h

SOURCES += \
    $$PWD/qevdevtablethandler.cpp \
    $$PWD/qevdevtabletmanager.cpp

contains(QT_CONFIG, libudev) {
    LIBS_PRIVATE += $$QMAKE_LIBS_LIBUDEV
}

HEADERS += \
    $$PWD/qevdevtouch_p.h

SOURCES += \
    $$PWD/qevdevtouch.cpp

contains(QT_CONFIG, libudev) {
    LIBS_PRIVATE += $$QMAKE_LIBS_LIBUDEV
}

# DEFINES += USE_MTDEV

contains(DEFINES, USE_MTDEV): LIBS += -lmtdev

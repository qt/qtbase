HEADERS += \
    $$PWD/qevdevtablethandler_p.h \
    $$PWD/qevdevtabletmanager_p.h

SOURCES += \
    $$PWD/qevdevtablethandler.cpp \
    $$PWD/qevdevtabletmanager.cpp

qtConfig(libudev): \
    QMAKE_USE_PRIVATE += libudev

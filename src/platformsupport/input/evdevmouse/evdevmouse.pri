HEADERS += \
    $$PWD/qevdevmousehandler_p.h \
    $$PWD/qevdevmousemanager_p.h

SOURCES += \
    $$PWD/qevdevmousehandler.cpp \
    $$PWD/qevdevmousemanager.cpp

qtConfig(libudev): \
    QMAKE_USE_PRIVATE += libudev


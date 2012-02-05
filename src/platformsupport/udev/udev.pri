contains(QT_CONFIG, libudev) {
    HEADERS += $$PWD/qudevhelper_p.h
    SOURCES += $$PWD/qudevhelper.cpp
    LIBS += -ludev
}

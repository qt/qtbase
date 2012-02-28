contains(QT_CONFIG, libudev) {
    HEADERS += $$PWD/qudevhelper_p.h $$PWD/qudevicehelper_p.h
    SOURCES += $$PWD/qudevhelper.cpp $$PWD/qudevicehelper.cpp
}

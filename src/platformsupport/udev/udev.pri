contains(QT_CONFIG, libudev) {
    HEADERS += $$PWD/qudevicehelper_p.h
    SOURCES += $$PWD/qudevicehelper.cpp

    INCLUDEPATH += $$QMAKE_INCDIR_LIBUDEV
    LIBS += $$QMAKE_LIBS_LIBUDEV
}

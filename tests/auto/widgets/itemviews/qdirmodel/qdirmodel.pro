CONFIG += testcase
TARGET = tst_qdirmodel
QT += widgets testlib
SOURCES         += tst_qdirmodel.cpp

INCLUDEPATH += ../../../../shared/
HEADERS += ../../../../shared/emulationdetector.h

android {
    DEFINES += SRCDIR=\\\"./\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

android:!android-embedded {
    RESOURCES += \
        testdata.qrc
}

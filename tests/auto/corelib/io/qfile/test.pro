CONFIG += testcase
QT = core-private testlib
qtHaveModule(network): QT += network
else: DEFINES += QT_NO_NETWORK

contains(CONFIG, builtin_testdata) {
    DEFINES += BUILTIN_TESTDATA
}

TESTDATA += BLACKLIST

TARGET = tst_qfile

SOURCES = tst_qfile.cpp
INCLUDEPATH += ../../../../shared/
HEADERS += ../../../../shared/emulationdetector.h

RESOURCES += qfile.qrc rename-fallback.qrc copy-fallback.qrc

TESTDATA += \
    dosfile.txt noendofline.txt testfile.txt \
    testlog.txt two.dots.file tst_qfile.cpp \
    Makefile forCopying.txt forRenaming.txt \
    resources/file1.ext1

win32:!winrt: QMAKE_USE += ole32 uuid

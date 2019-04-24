CONFIG += testcase
TARGET = tst_qfileinfo
QT = core-private testlib
SOURCES = tst_qfileinfo.cpp
RESOURCES += qfileinfo.qrc \
    testdata.qrc

win32:!winrt: QMAKE_USE += advapi32 netapi32

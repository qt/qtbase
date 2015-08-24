CONFIG += testcase
TARGET = tst_qfileinfo
QT = core-private testlib
SOURCES = tst_qfileinfo.cpp
RESOURCES += qfileinfo.qrc \
    testdata.qrc

win32:!wince:!winrt:LIBS += -ladvapi32 -lnetapi32

win32: CONFIG += insignificant_test # Crashes on Windows in release builds

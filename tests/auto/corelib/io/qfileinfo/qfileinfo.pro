CONFIG += testcase
TARGET = tst_qfileinfo
QT = core-private testlib
SOURCES = tst_qfileinfo.cpp
RESOURCES += qfileinfo.qrc \
    testdata.qrc

win32: LIBS += -ladvapi32 -lnetapi32

# for std::filesystem tests
qtConfig(c++17): CONFIG += c++17

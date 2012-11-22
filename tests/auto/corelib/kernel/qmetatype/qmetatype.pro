CONFIG += testcase parallel_test
TARGET = tst_qmetatype
QT = core testlib
SOURCES = tst_qmetatype.cpp
TESTDATA=./typeFlags.bin
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

win32-msvc2008 {
    # Prevents "fatal error C1128: number of sections exceeded object file format limit".
    QMAKE_CXXFLAGS += /bigobj
}
CONFIG += testcase parallel_test
TARGET = tst_qtendian
QT = core testlib
SOURCES = tst_qtendian.cpp
wince* { # QTBUG-37194 , internal compiler errors with MSVC2008 for Windows CE
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE -= -O2
}
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

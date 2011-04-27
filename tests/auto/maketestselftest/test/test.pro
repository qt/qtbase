load(qttest_p4)

TARGET = ../tst_maketestselftest
SOURCES += ../tst_maketestselftest.cpp
QT = core

DEFINES += SRCDIR=\\\"$$PWD/..\\\"

requires(!cross_compile)

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_maketestselftest
} else {
    TARGET = ../../release/tst_maketestselftest
  }
}


load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TARGET = tst_bench_qtextcodec
QT -= gui
SOURCES += main.cpp

wince*:{
   DEFINES += SRCDIR=\\\"\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}


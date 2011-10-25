TARGET = tst_bench_qtextcodec
QT = core testlib
SOURCES += main.cpp

wince*:{
   DEFINES += SRCDIR=\\\"\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}


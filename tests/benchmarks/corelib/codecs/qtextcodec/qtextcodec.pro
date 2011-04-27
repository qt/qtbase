load(qttest_p4)
TARGET = tst_bench_qtextcodec
QT -= gui
SOURCES += main.cpp

wince*:{
   DEFINES += SRCDIR=\\\"\\\"
} else:symbian* {
   addFiles.files = utf-8.txt
   addFiles.path = .
   DEPLOYMENT += addFiles
   TARGET.EPOCHEAPSIZE="0x100 0x1000000"
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}


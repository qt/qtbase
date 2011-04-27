load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_QText

SOURCES += main.cpp

symbian* {
   TARGET.CAPABILITY = ALL -TCB
   addFiles.files = bidi.txt
   addFiles.path = .
   DEPLOYMENT += addFiles
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
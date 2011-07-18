load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

QT += gui-private

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

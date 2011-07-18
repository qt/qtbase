load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TARGET = tst_bench_qstringlist
CONFIG -= debug
CONFIG += release
QT -= gui
SOURCES += main.cpp

symbian: LIBS += -llibpthread

load(qttest_p4)
TARGET = tst_bench_qstringlist
CONFIG -= debug
CONFIG += release
QT -= gui
SOURCES += main.cpp

symbian: LIBS += -llibpthread

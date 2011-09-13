load(qttest_p4)
SOURCES += tst_qthread.cpp
QT = core
symbian:LIBS += -llibpthread
CONFIG += parallel_test

load(qttest_p4)
DEFINES += QT_STRICT_ITERATORS
SOURCES += tst_qfuture.cpp
QT = core core-private
CONFIG += parallel_test

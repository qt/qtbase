load(qttest_p4)
DEFINES += QT_STRICT_ITERATORS
SOURCES += tst_qtconcurrentfilter.cpp
QT = core
CONFIG += parallel_test
CONFIG += insignificant_test # See QTBUG-20688

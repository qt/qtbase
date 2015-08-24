CONFIG += testcase parallel_test
TARGET = tst_qurl
QT = core testlib concurrent
SOURCES = tst_qurl.cpp

mac: OBJECTIVE_SOURCES += tst_qurl_mac.mm

CONFIG += testcase parallel_test
TARGET = tst_qfilesystemwatcher
QT = core testlib
SOURCES = tst_qfilesystemwatcher.cpp

mac: CONFIG += insignificant_test # QTBUG-22744
win32:CONFIG += insignificant_test # QTBUG-24029

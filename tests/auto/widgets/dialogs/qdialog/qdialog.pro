CONFIG += testcase
TARGET = tst_qdialog
QT += widgets testlib
SOURCES += tst_qdialog.cpp
mac:CONFIG += insignificant_test    # QTBUG-24977

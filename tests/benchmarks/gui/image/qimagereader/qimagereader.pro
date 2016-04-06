QT += testlib

TEMPLATE = app
TARGET = tst_bench_qimagereader

SOURCES += tst_qimagereader.cpp

contains(QT_CONFIG, gif):DEFINES += QTEST_HAVE_GIF
contains(QT_CONFIG, jpeg):DEFINES += QTEST_HAVE_JPEG
QT += network

QT += testlib

TEMPLATE = app
TARGET = tst_bench_qimagereader

SOURCES += tst_qimagereader.cpp

qtConfig(gif): DEFINES += QTEST_HAVE_GIF
qtConfig(jpeg): DEFINES += QTEST_HAVE_JPEG
QT += network

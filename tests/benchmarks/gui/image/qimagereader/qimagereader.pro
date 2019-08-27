QT += testlib
QT_FOR_CONFIG += gui-private

TEMPLATE = app
TARGET = tst_bench_qimagereader

SOURCES += tst_qimagereader.cpp

qtConfig(gif): DEFINES += QTEST_HAVE_GIF
qtConfig(jpeg): DEFINES += QTEST_HAVE_JPEG

TESTDATA += images/*

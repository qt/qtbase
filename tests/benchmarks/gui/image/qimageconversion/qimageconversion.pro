TEMPLATE = app
TARGET = tst_bench_imageConversion
QT += testlib
QT_FOR_CONFIG += gui-private
SOURCES += tst_qimageconversion.cpp

qtConfig(gif): DEFINES += QTEST_HAVE_GIF
qtConfig(jpeg): DEFINES += QTEST_HAVE_JPEG
qtConfig(c++11): CONFIG += c++11

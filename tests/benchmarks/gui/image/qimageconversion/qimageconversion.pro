TEMPLATE = app
TARGET = tst_bench_imageConversion
QT += testlib
SOURCES += tst_qimageconversion.cpp

!contains(QT_CONFIG, no-gif):DEFINES += QTEST_HAVE_GIF
!contains(QT_CONFIG, no-jpeg):DEFINES += QTEST_HAVE_JPEG
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

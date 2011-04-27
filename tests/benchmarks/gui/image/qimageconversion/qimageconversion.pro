load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_imageConversion

SOURCES += tst_qimageconversion.cpp

!contains(QT_CONFIG, no-gif):DEFINES += QTEST_HAVE_GIF
!contains(QT_CONFIG, no-jpeg):DEFINES += QTEST_HAVE_JPEG
!contains(QT_CONFIG, no-mng):DEFINES += QTEST_HAVE_MNG
!contains(QT_CONFIG, no-tiff):DEFINES += QTEST_HAVE_TIFF

CONFIG += testcase
TARGET = tst_qmovie
QT += testlib
qtHaveModule(widgets): QT += widgets
SOURCES += tst_qmovie.cpp
MOC_DIR=tmp

!contains(QT_CONFIG, no-gif):DEFINES += QTEST_HAVE_GIF
!contains(QT_CONFIG, no-jpeg):DEFINES += QTEST_HAVE_JPEG

RESOURCES += resources.qrc
TESTDATA += animations/*

CONFIG += testcase
TARGET = tst_qmovie
QT += testlib
QT_FOR_CONFIG += gui-private
qtHaveModule(widgets): QT += widgets
SOURCES += tst_qmovie.cpp
MOC_DIR=tmp

qtConfig(gif): DEFINES += QTEST_HAVE_GIF
qtConfig(jpeg): DEFINES += QTEST_HAVE_JPEG

RESOURCES += resources.qrc
TESTDATA += animations/*

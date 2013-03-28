CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qrawfont

QT = core core-private gui gui-private testlib

SOURCES += \
    tst_qrawfont.cpp

TESTDATA += testfont_bold_italic.ttf  testfont.ttf

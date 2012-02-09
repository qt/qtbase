CONFIG += testcase
TARGET = tst_qrawfont

QT = core core-private gui gui-private testlib

SOURCES += \
    tst_qrawfont.cpp

INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

TESTDATA += testfont_bold_italic.ttf  testfont.ttf

win32:CONFIG += insignificant_test # QTBUG-24197

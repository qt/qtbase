CONFIG += testcase
TARGET = tst_qdatetimeedit
QT += widgets testlib
SOURCES  += tst_qdatetimeedit.cpp

wincewm50smart-msvc2005: DEFINES += WINCE_NO_MODIFIER_KEYS
mac:CONFIG += insignificant_test # numerous failures, see QTBUG-23674

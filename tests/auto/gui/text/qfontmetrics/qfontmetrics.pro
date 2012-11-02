CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qfontmetrics
QT += testlib core-private gui-private
SOURCES  += tst_qfontmetrics.cpp
RESOURCES += testfont.qrc
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

CONFIG += testcase
TARGET = tst_qmenubar
QT += widgets testlib
SOURCES += tst_qmenubar.cpp

# QTBUG-4965, QTBUG-11823 - unstable tests
linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):CONFIG += insignificant_test

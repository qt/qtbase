CONFIG += testcase
TARGET = tst_qdoublespinbox
QT += widgets testlib
SOURCES  += tst_qdoublespinbox.cpp

linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):CONFIG += insignificant_test # QTBUG-23641

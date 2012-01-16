CONFIG += testcase
TARGET = tst_qdoublespinbox
QT += widgets testlib
SOURCES  += tst_qdoublespinbox.cpp

# QTBUG-23641 - unstable test
linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):CONFIG += insignificant_test

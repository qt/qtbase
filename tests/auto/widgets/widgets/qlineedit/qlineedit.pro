CONFIG += testcase
TARGET = tst_qlineedit
QT += gui-private core-private widgets testlib
SOURCES  += tst_qlineedit.cpp

# QTBUG-24518 - unstable test
linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):DEFINES+=UBUNTU_ONEIRIC

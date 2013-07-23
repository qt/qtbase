CONFIG += testcase parallel_test
TARGET = tst_qeasingcurve
QT = core testlib
SOURCES = tst_qeasingcurve.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):DEFINES+=UBUNTU_ONEIRIC # QTBUG-32432

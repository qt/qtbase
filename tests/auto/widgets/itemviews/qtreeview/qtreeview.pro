CONFIG += testcase
TARGET = tst_qtreeview
QT += widgets testlib
QT += widgets-private gui-private core-private
SOURCES  += tst_qtreeview.cpp

# QTBUG-23638
linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):CONFIG+=insignificant_test

load(qttest_p4)
SOURCES  += tst_qprinterinfo.cpp

QT += printsupport network

DEFINES += QT_USE_USING_NAMESPACE

CONFIG += insignificant_test # QTBUG-21402

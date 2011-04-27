############################################################
# Project file for autotest for file qnetworkproxy.h
############################################################

load(qttest_p4)
QT = core network

SOURCES += tst_qnetworkproxy.cpp

symbian: TARGET.CAPABILITY = NetworkServices


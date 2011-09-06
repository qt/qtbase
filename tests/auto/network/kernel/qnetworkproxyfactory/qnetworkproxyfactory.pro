############################################################
# Project file for autotest for file qnetworkproxy.h (proxy factory part)
############################################################

load(qttest_p4)
QT = core network

SOURCES += tst_qnetworkproxyfactory.cpp

symbian: TARGET.CAPABILITY = NetworkServices ReadUserData


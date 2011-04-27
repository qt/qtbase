############################################################
# Project file for autotest for file qabstractsocket.h
############################################################

load(qttest_p4)
QT = core network

SOURCES += tst_qabstractsocket.cpp

symbian: TARGET.CAPABILITY = NetworkServices


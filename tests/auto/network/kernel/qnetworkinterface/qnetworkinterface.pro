CONFIG += testcase
TARGET = tst_qnetworkinterface
SOURCES  += tst_qnetworkinterface.cpp

QT = core network testlib

win32:CONFIG+=insignificant_test      # QTBUG-24451 - localAddress()

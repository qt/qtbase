load(qttest_p4)
QT -= gui
QT += network
SOURCES  += tst_qnetworkcachemetadata.cpp

symbian: TARGET.CAPABILITY = NetworkServices


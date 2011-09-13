load(qttest_p4)
QT -= gui
QT += network
SOURCES  += tst_qnetworkdiskcache.cpp

symbian: TARGET.CAPABILITY = NetworkServices


load(qttest_p4)
SOURCES  += tst_qnetworkrequest.cpp

QT = core network
symbian: TARGET.CAPABILITY = NetworkServices

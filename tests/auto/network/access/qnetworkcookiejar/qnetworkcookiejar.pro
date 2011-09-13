load(qttest_p4)
SOURCES  += tst_qnetworkcookiejar.cpp

QT = core core-private network network-private
symbian: TARGET.CAPABILITY = NetworkServices

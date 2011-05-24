load(qttest_p4)
SOURCES  += tst_qurl.cpp
QT = core core-private
symbian: TARGET.CAPABILITY = NetworkServices
CONFIG += parallel_test

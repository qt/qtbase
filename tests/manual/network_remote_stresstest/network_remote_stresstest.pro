TARGET = tst_network_remote_stresstest

QT = core core-private network network-private testlib

SOURCES  += tst_network_remote_stresstest.cpp

RESOURCES += url-list.qrc

LIBS += $$QMAKE_LIBS_NETWORK

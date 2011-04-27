load(qttest_p4)
SOURCES  += tst_qvariant.cpp
QT += network

contains(QT_CONFIG, qt3support): QT += qt3support

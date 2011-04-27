load(qttest_p4)
SOURCES  += tst_qalgorithms.cpp

QT = core
contains(QT_CONFIG, qt3support): QT += qt3support

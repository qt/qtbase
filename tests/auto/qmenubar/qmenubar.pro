load(qttest_p4)
HEADERS += 
SOURCES += tst_qmenubar.cpp

contains(QT_CONFIG, qt3support):!symbian:QT += qt3support


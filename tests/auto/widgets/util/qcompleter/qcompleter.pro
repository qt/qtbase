load(qttest_p4)
TEMPLATE = app
TARGET = tst_qcompleter
QT += widgets
DEPENDPATH += .
INCLUDEPATH += . ..

# Input
SOURCES += tst_qcompleter.cpp

CONFIG += insignificant_test # QTBUG-21424

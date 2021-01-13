CONFIG += testcase
TARGET = tst_qplaintextedit

QT += widgets widgets-private testlib
QT += gui-private

INCLUDEPATH += ../

HEADERS +=  
SOURCES += tst_qplaintextedit.cpp 

osx: LIBS += -framework AppKit

winrt:CONFIG += insignificant_test #QTBUG-90441

load(qttest_p4)
TEMPLATE = app
TARGET = qtimer_vs_qmetaobject
DEPENDPATH += .
INCLUDEPATH += .

CONFIG += release
#CONFIG += debug


SOURCES += tst_qtimer_vs_qmetaobject.cpp
QT -= gui

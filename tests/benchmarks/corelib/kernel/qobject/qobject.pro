load(qttest_p4)
QT += widgets
TEMPLATE = app
TARGET = tst_bench_qobject
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += object.h
SOURCES += main.cpp object.cpp

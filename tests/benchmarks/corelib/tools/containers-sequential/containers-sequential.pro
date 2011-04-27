load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_containers-sequential
DEPENDPATH += .
INCLUDEPATH += .

# Input
SOURCES += main.cpp
QT -= gui

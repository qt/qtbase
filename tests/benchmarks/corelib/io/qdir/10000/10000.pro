load(qttest_p4)
TEMPLATE = app
TARGET = bench_qdir_10000
DEPENDPATH += .
INCLUDEPATH += .

# Input
SOURCES += bench_qdir_10000.cpp

QT -= gui

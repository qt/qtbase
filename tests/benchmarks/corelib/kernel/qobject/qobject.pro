load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_qobject
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += object.h
SOURCES += main.cpp object.cpp

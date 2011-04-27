load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_qanimation
DEPENDPATH += .
INCLUDEPATH += .

CONFIG += release
#CONFIG += debug


SOURCES += main.cpp \
           dummyobject.cpp \
           dummyanimation.cpp \
           rectanimation.cpp

HEADERS += dummyobject.h \
           dummyanimation.h \
           rectanimation.h

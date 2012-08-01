QT += widgets testlib

TARGET = tst_bench_qanimation

CONFIG += release
#CONFIG += debug


SOURCES += main.cpp \
           dummyobject.cpp \
           dummyanimation.cpp \
           rectanimation.cpp

HEADERS += dummyobject.h \
           dummyanimation.h \
           rectanimation.h
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

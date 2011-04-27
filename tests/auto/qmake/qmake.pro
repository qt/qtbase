load(qttest_p4)
HEADERS += testcompiler.h
SOURCES += tst_qmake.cpp testcompiler.cpp
QT -= gui

cross_compile: DEFINES += QMAKE_CROSS_COMPILED



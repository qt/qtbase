CONFIG += testcase
TARGET = tst_qline
QT = core testlib
SOURCES = tst_qline.cpp
unix:!darwin:!vxworks:!haiku:!integrity: LIBS+=-lm

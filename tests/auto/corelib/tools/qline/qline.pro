CONFIG += testcase
TARGET = tst_qline
QT = core testlib
SOURCES += tst_qline.cpp
unix:!mac:!vxworks:LIBS+=-lm

CONFIG += parallel_test

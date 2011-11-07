CONFIG += testcase parallel_test
TARGET = tst_qline
QT = core testlib
SOURCES = tst_qline.cpp
unix:!mac:!vxworks:LIBS+=-lm

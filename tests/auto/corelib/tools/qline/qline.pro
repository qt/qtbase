CONFIG += testcase
TARGET = tst_qline
QT = core testlib
SOURCES = tst_qline.cpp
unix:!mac:!vxworks:!haiku:LIBS+=-lm

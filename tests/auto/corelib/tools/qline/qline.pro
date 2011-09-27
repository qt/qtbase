load(qttest_p4)
QT -= gui
SOURCES += tst_qline.cpp
unix:!mac:!vxworks:LIBS+=-lm

CONFIG += parallel_test

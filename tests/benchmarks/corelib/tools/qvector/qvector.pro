TARGET = tst_bench_vector
QT = core testlib core-private
INCLUDEPATH += .
SOURCES += main.cpp outofline.cpp 
CONFIG += release
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

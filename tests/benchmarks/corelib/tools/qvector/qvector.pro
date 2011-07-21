load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TARGET = tst_vector
QT = core
INCLUDEPATH += .
SOURCES += main.cpp outofline.cpp 
CONFIG += release

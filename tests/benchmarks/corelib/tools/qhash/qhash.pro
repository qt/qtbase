load(qttest_p4)

# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TARGET = tst_hash
QT = core
INCLUDEPATH += .
SOURCES += qhash_string.cpp outofline.cpp 
CONFIG += release

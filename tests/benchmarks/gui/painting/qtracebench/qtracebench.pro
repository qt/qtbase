load(qttest_p4)

QT += widgets
QT += core-private gui-private widgets-private
# do not run benchmarks by default in 'make check'
CONFIG -= testcase

TEMPLATE = app
TARGET = tst_qtracebench

INCLUDEPATH += . $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

RESOURCES += qtracebench.qrc

SOURCES += tst_qtracebench.cpp


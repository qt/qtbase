QT += widgets testlib
QT += core-private gui-private widgets-private

TEMPLATE = app
TARGET = tst_qtracebench

INCLUDEPATH += . $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

RESOURCES += qtracebench.qrc

SOURCES += tst_qtracebench.cpp


load(qttest_p4)
TEMPLATE = app
TARGET = tst_qtracebench

INCLUDEPATH += . $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

RESOURCES += qtracebench.qrc

SOURCES += tst_qtracebench.cpp


load(qttest_p4)
TEMPLATE = app
TARGET = bench_qdir_tree
DEPENDPATH += .
INCLUDEPATH += .

# Input
SOURCES += bench_qdir_tree.cpp
RESOURCES += bench_qdir_tree.qrc

QT -= gui

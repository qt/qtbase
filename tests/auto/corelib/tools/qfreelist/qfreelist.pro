CONFIG += testcase
TARGET = tst_qfreelist
QT = core-private testlib
SOURCES = tst_qfreelist.cpp
!contains(QT_CONFIG,private_tests): SOURCES += $$QT_SOURCE_TREE/src/corelib/tools/qfreelist.cpp

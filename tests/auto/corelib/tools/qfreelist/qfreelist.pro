CONFIG += testcase
TARGET = tst_qfreelist
SOURCES  += tst_qfreelist.cpp
QT += core-private testlib
QT -= gui
!contains(QT_CONFIG,private_tests): SOURCES += $$QT.core.sources/tools/qfreelist.cpp

CONFIG += testcase
TARGET = tst_qfreelist
QT = core-private testlib
SOURCES = tst_qfreelist.cpp
!contains(QT_CONFIG,private_tests): SOURCES += $$QT.core.sources/tools/qfreelist.cpp

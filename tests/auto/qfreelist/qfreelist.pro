load(qttest_p4)
SOURCES  += tst_qfreelist.cpp
QT += core-private
QT -= gui

!private_tests:SOURCES += $$QT_SOURCE_TREE/src/corelib/tools/qfreelist.cpp

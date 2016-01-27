CONFIG += testcase console
CONFIG += parallel_test
TARGET = ../tst_qobject
QT = core-private network testlib
SOURCES = ../tst_qobject.cpp

!winrt: TEST_HELPER_INSTALLS = ../signalbug/signalbug
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

CONFIG += testcase parallel_test
TARGET = tst_qnumeric
QT = core-private testlib
SOURCES = tst_qnumeric.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
intel_icc: QMAKE_CXXFLAGS += -fp-model strict
intel_icl: QMAKE_CXXFLAGS += /fp:strict

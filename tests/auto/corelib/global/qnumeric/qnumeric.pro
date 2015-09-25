CONFIG += testcase
TARGET = tst_qnumeric
QT = core-private testlib
SOURCES = tst_qnumeric.cpp
intel_icc: QMAKE_CXXFLAGS += -fp-model strict
intel_icl: QMAKE_CXXFLAGS += /fp:strict

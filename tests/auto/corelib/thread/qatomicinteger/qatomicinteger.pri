# Get our build type from the directory name
TYPE = $$basename(_PRO_FILE_PWD_)
dn = $$dirname(_PRO_FILE_PWD_)
FORCE = $$basename(dn)
suffix = $$TYPE

CONFIG += testcase
QT = core testlib
TARGET = tst_qatomicinteger_$$lower($$suffix)
SOURCES = $$PWD/tst_qatomicinteger.cpp
DEFINES += QATOMIC_TEST_TYPE=$$TYPE tst_QAtomicIntegerXX=tst_QAtomicInteger_$$suffix

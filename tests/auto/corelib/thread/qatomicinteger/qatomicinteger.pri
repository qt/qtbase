# Get our build type from the directory name
TYPE = $$basename(_PRO_FILE_PWD_)
dn = $$dirname(_PRO_FILE_PWD_)
FORCE = $$basename(dn)

equals(FORCE, no-cxx11) {
    suffix = NoCxx11_$$TYPE
    DEFINES += QT_ATOMIC_FORCE_NO_CXX11
} else: equals(FORCE, gcc) {
    suffix = Gcc_$$TYPE
    DEFINES += QT_ATOMIC_FORCE_GCC
} else {
    suffix = $$TYPE
}

CONFIG += testcase parallel_test
QT = core testlib
TARGET = tst_qatomicinteger_$$lower($$suffix)
SOURCES = $$PWD/tst_qatomicinteger.cpp
DEFINES += QATOMIC_TEST_TYPE=$$TYPE tst_QAtomicIntegerXX=tst_QAtomicInteger_$$suffix

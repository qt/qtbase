load(qttest_p4)
SOURCES  += tst_qset.cpp
QT = core

symbian: {
TARGET.EPOCSTACKSIZE =0x5000
TARGET.EPOCHEAPSIZE="0x100000 0x1000000" # // Min 1Mb, max 16Mb
}
CONFIG += parallel_test

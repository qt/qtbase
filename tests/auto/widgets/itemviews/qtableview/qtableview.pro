CONFIG += testcase

QT += widgets widgets-private testlib
QT += core-private gui-private

TARGET.EPOCHEAPSIZE = 0x200000 0x800000
SOURCES  += tst_qtableview.cpp

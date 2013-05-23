CONFIG += testcase
TARGET = tst_qtableview
QT += widgets widgets-private testlib
QT += core-private gui-private

TARGET.EPOCHEAPSIZE = 0x200000 0x800000
SOURCES  += tst_qtableview.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

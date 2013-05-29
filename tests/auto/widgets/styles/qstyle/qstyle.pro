CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qstyle
TARGET.EPOCHEAPSIZE = 0x200000 0x800000
QT += widgets testlib
SOURCES  += tst_qstyle.cpp

wince* {
   addPixmap.files = task_25863.png
   addPixmap.path = .
   DEPLOYMENT += addPixmap
}

load(qttest_p4)
TARGET.EPOCHEAPSIZE = 0x200000 0x800000
SOURCES  += tst_qstyle.cpp

wince*|symbian: {
   !symbian:DEFINES += SRCDIR=\\\".\\\"
   addPixmap.files = task_25863.png
   addPixmap.path = .
   DEPLOYMENT += addPixmap
} else {
   DEFINES += SRCDIR=\\\"$$PWD\\\"
}

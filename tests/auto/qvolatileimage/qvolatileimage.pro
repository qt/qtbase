load(qttest_p4)

QT += gui-private

SOURCES  += tst_qvolatileimage.cpp

symbian {
   TARGET.EPOCHEAPSIZE = 0x200000 0x800000
   LIBS += -lfbscli
}

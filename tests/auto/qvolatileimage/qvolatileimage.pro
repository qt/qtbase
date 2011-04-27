load(qttest_p4)
SOURCES  += tst_qvolatileimage.cpp

symbian {
   TARGET.EPOCHEAPSIZE = 0x200000 0x800000
   LIBS += -lfbscli
}

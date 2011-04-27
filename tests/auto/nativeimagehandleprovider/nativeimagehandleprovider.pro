load(qttest_p4)
SOURCES  += tst_nativeimagehandleprovider.cpp
symbian {
    LIBS += -lfbscli -lbitgdi
    contains(QT_CONFIG, openvg): QT *= openvg
}

load(qttest_p4)
QT -= gui
SOURCES  += ../tst_qnetworkreply.cpp
TARGET = ../tst_qnetworkreply

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qnetworkreply
} else {
    TARGET = ../../release/tst_qnetworkreply
  }
}

!symbian:DEFINES += SRCDIR=\\\"$$PWD/..\\\"

QT = core-private network-private
RESOURCES += ../qnetworkreply.qrc

symbian|wince*:{
    # For cross compiled targets, reference data files need to be deployed
    addFiles.files = ../empty ../rfc3252.txt ../resource ../bigfile ../*.jpg
    addFiles.path = .
    DEPLOYMENT += addFiles

    certFiles.files = ../certs
    certFiles.path    = .
    DEPLOYMENT += certFiles
}

symbian:{
    # Symbian toolchain does not support correct include semantics
    INCLUDEPATH+=..\\..\\..\\..\\include\\QtNetwork\\private
    # bigfile test case requires more heap
    TARGET.EPOCHEAPSIZE="0x100 0x10000000"
    TARGET.CAPABILITY="ALL -TCB"
}

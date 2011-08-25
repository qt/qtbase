load(qttest_p4)
SOURCES  += tst_qftp.cpp


QT = core network network-private

wince*: {
   addFiles.files = rfc3252.txt
   addFiles.path = .
   DEPLOYMENT += addFiles
   DEFINES += SRCDIR=\\\"\\\"
} else:symbian {
   addFiles.files = rfc3252.txt
   addFiles.path = .
   DEPLOYMENT += addFiles
   TARGET.EPOCHEAPSIZE="0x100 0x1000000"
   TARGET.CAPABILITY = NetworkServices
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

CONFIG+=insignificant_test  # uses live qt-test-server, inherently unstable

load(qttest_p4)
SOURCES  += tst_qiodevice.cpp

QT = core network

wince*: {
   addFiles.files = tst_qiodevice.cpp
   addFiles.path = .
   DEPLOYMENT += addFiles
   DEFINES += SRCDIR=\\\"\\\"
   !wince50standard-x86-msvc2005: DEFINES += WINCE_EMULATOR_TEST=1
} else:symbian {
   # SRCDIR defined in code in symbian
   addFiles.files = tst_qiodevice.cpp
   addFiles.path = .
   DEPLOYMENT += addFiles
   TARGET.CAPABILITY = NetworkServices
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
   contains(QT_CONFIG, qt3support):QT += qt3support
}
MOC_DIR=tmp



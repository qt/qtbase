load(qttest_p4)
SOURCES += tst_qdatastream.cpp
QT += gui widgets
wince*: {
   addFiles.files = datastream.q42
   addFiles.path = .
   DEPLOYMENT += addFiles
   DEFINES += SRCDIR=\\\"\\\"
} else:symbian {
   # SRCDIR defined in code in symbian
   addFiles.files = datastream.q42
   addFiles.path = .
   DEPLOYMENT += addFiles
   TARGET.EPOCHEAPSIZE = 1000000 10000000
   TARGET.UID3 = 0xE0340001
   DEFINES += SYMBIAN_SRCDIR_UID=$$lower($$replace(TARGET.UID3,"0x",""))
}else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}


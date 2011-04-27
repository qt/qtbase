load(qttest_p4)
SOURCES += tst_qdatastream.cpp

!symbian: {
cross_compile: DEFINES += SVGFILE=\\\"tests2.svg\\\"
else: DEFINES += SVGFILE=\\\"gearflowers.svg\\\"
}

# for qpaintdevicemetrics.h
contains(QT_CONFIG, qt3support):QT += qt3support
QT += svg


wince*: {
   addFiles.files = datastream.q42 tests2.svg
   addFiles.path = .
   DEPLOYMENT += addFiles
   DEFINES += SRCDIR=\\\"\\\"
} else:symbian {
   # SRCDIR and SVGFILE defined in code in symbian
   addFiles.files = datastream.q42 tests2.svg
   addFiles.path = .
   DEPLOYMENT += addFiles
   TARGET.EPOCHEAPSIZE = 1000000 10000000
   TARGET.UID3 = 0xE0340001
   DEFINES += SYMBIAN_SRCDIR_UID=$$lower($$replace(TARGET.UID3,"0x",""))
}else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}


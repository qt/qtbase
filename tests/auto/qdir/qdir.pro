load(qttest_p4)
SOURCES   += tst_qdir.cpp
RESOURCES += qdir.qrc
QT        = core

wince*|symbian {
  DirFiles.files = testdir testData searchdir resources entrylist types tst_qdir.cpp
  DirFiles.path = .
  DEPLOYMENT += DirFiles
}

wince* {
  DEFINES += SRCDIR=\\\"\\\"
} else:symbian {
  TARGET.CAPABILITY += AllFiles
  TARGET.UID3 = 0xE0340002
  DEFINES += SYMBIAN_SRCDIR_UID=$$lower($$replace(TARGET.UID3,"0x",""))
  LIBS += -lefsrv
  INCLUDEPATH += $$MW_LAYER_SYSTEMINCLUDE
} else {
  contains(QT_CONFIG, qt3support):QT += qt3support
  DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

CONFIG += parallel_test

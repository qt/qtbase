############################################################
# Project file for autotest for file qfiledialog.h
############################################################

load(qttest_p4)

SOURCES += tst_qfiledialog.cpp

wince*|symbian {
    addFiles.files = *.cpp
    addFiles.path = .
    filesInDir.files = *.pro
    filesInDir.path = someDir
    DEPLOYMENT += addFiles filesInDir
}

symbian:TARGET.EPOCHEAPSIZE="0x100 0x1000000"
symbian:HEADERS += ../../../include/qtgui/private/qfileinfogatherer_p.h

wince* {
    DEFINES += SRCDIR=\\\"./\\\"
} else:symbian {
    TARGET.UID3 = 0xE0340003
    DEFINES += SYMBIAN_SRCDIR_UID=$$lower($$replace(TARGET.UID3,"0x",""))
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

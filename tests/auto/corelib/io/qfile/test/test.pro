load(qttest_p4)
SOURCES  += ../tst_qfile.cpp

wince*|symbian {
    QT = core gui
    files.files += ..\\dosfile.txt ..\\noendofline.txt ..\\testfile.txt \
                     ..\\testlog.txt ..\\two.dots.file ..\\tst_qfile.cpp \
                     ..\\Makefile ..\\forCopying.txt ..\\forRenaming.txt
    files.path = .
    resour.files += ..\\resources\\file1.ext1
    resour.path = resources

    DEPLOYMENT += files resour
}

wince* {
    SOURCES += $$QT_SOURCE_TREE/src/corelib/kernel/qfunctions_wince.cpp     # needed for QT_OPEN
    DEFINES += SRCDIR=\\\"\\\"
} else:symbian {
    # do not define SRCDIR at all
    TARGET.EPOCHEAPSIZE = 0x100000 0x3000000
} else {
    QT = core network
    DEFINES += SRCDIR=\\\"$$PWD/../\\\"
}

RESOURCES      += ../qfile.qrc ../rename-fallback.qrc ../copy-fallback.qrc

TARGET = ../tst_qfile

win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qfile
    } else {
        TARGET = ../../release/tst_qfile
    }
    LIBS+=-lole32 -luuid
}

symbian {
    LIBS+=-lefsrv
}

mac*:CONFIG+=insignificant_test

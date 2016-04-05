CONFIG += testcase
TARGET = tst_qdirmodel
QT += widgets testlib
SOURCES         += tst_qdirmodel.cpp

wince* {
	addit.files = dirtest\\test1\\*
	addit.path = dirtest\\test1
	tests.files = test\\*
	tests.path = test
        sourceFile.files = tst_qdirmodel.cpp
        sourceFile.path = .
	DEPLOYMENT += addit tests sourceFile
}

android|wince {
    DEFINES += SRCDIR=\\\"./\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

android {
    RESOURCES += \
        testdata.qrc
}

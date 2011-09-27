load(qttest_p4)
SOURCES += tst_qxmlstream.cpp

QT = core xml network

wince* {
    addFiles.files = data XML-Test-Suite
    addFiles.path = .
    DEPLOYMENT += addFiles
    wince*:DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

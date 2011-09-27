load(qttest_p4)
SOURCES  += tst_qfontdatabase.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"

wince* {
    additionalFiles.files = FreeMono.ttf
    additionalFiles.path = .
    DEPLOYMENT += additionalFiles
}


load(qttest_p4)

INCLUDEPATH += ../

HEADERS +=  
SOURCES += tst_qtextedit.cpp 

wince*|symbian: {
    addImages.files = fullWidthSelection/*
    addImages.path = fullWidthSelection
    DEPLOYMENT += addImages
}

wince* {
    DEFINES += SRCDIR=\\\"./\\\"
} else:!symbian {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

contains(QT_CONFIG, qt3support): QT += qt3support

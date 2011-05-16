load(qttest_p4)

QT += gui-private
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

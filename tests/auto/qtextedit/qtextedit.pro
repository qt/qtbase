load(qttest_p4)

QT += widgets widgets-private gui-private
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

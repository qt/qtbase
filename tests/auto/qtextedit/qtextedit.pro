load(qttest_p4)

QT += widgets widgets-private gui-private
INCLUDEPATH += ../

HEADERS +=  
SOURCES += tst_qtextedit.cpp 

wince* {
    addImages.files = fullWidthSelection/*
    addImages.path = fullWidthSelection
    DEPLOYMENT += addImages
    DEFINES += SRCDIR=\\\"./\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

contains(QT_CONFIG,xcb):qpa:CONFIG+=insignificant_test  # QTBUG-20756 crashes on qpa, xcb

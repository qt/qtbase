load(qttest_p4)

QT += gui-private

contains(QT_CONFIG, qt3support): QT += qt3support
SOURCES  += tst_qpainter.cpp
wince*|symbian: {
    addFiles.files = drawEllipse drawLine_rop_bitmap drawPixmap_rop drawPixmap_rop_bitmap task217400.png
    addFiles.path = .
    DEPLOYMENT += addFiles
}

wince* {
    DEFINES += SRCDIR=\\\".\\\"
} else:!symbian {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}



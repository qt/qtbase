load(qttest_p4)
QT += core-private gui-private
SOURCES  += tst_qgraphicsscene.cpp
RESOURCES += images.qrc
win32:!wince*: LIBS += -lUser32

!wince*:!symbian:DEFINES += SRCDIR=\\\"$$PWD\\\"
DEFINES += QT_NO_CAST_TO_ASCII

wince*|symbian: {
   rootFiles.files = Ash_European.jpg graphicsScene_selection.data
   rootFiles.path = .
   renderFiles.files = testData\\render\\*
   renderFiles.path = testData\\render
   DEPLOYMENT += rootFiles renderFiles
}
wince*:{
   DEFINES += SRCDIR=\\\".\\\"
}

symbian:TARGET.EPOCHEAPSIZE="0x100000 0x1000000" # Min 1Mb, max 16Mb

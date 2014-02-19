CONFIG += testcase
TARGET = tst_qgraphicsscene
QT += widgets widgets-private testlib
QT += core-private gui-private
SOURCES  += tst_qgraphicsscene.cpp
RESOURCES += images.qrc
win32:!wince*:!winrt: LIBS += -luser32

!wince*:DEFINES += SRCDIR=\\\"$$PWD\\\"
DEFINES += QT_NO_CAST_TO_ASCII

wince* {
   rootFiles.files = Ash_European.jpg graphicsScene_selection.data
   rootFiles.path = .
   renderFiles.files = testData\\render\\*
   renderFiles.path = testData\\render
   DEPLOYMENT += rootFiles renderFiles
   DEFINES += SRCDIR=\\\".\\\"
}
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

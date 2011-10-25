CONFIG += testcase
TARGET = tst_qpixmap

QT += core-private gui-private widgets widgets-private testlib

SOURCES  += tst_qpixmap.cpp
wince* {
   task31722_0.files = convertFromImage/task31722_0/*.png
   task31722_0.path    = convertFromImage/task31722_0

   task31722_1.files = convertFromImage/task31722_1/*.png
   task31722_1.path    = convertFromImage/task31722_1
 
   icons.files = convertFromToHICON/*       
   icons.path = convertFromToHICON
   
   loadFromData.files = loadFromData/*
   loadFromData.path = loadFromData

   DEPLOYMENT += task31722_0 task31722_1 icons loadFromData
   DEFINES += SRCDIR=\\\".\\\"
   DEPLOYMENT_PLUGIN += qico
} else {
   DEFINES += SRCDIR=\\\"$$PWD\\\"
   win32:LIBS += -lgdi32 -luser32
}

RESOURCES += qpixmap.qrc

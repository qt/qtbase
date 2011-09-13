load(qttest_p4)

QT += core-private gui-private widgets widgets-private

SOURCES  += tst_qpixmap.cpp
wince*|symbian: {

   task31722_0.files = convertFromImage/task31722_0/*.png
   task31722_0.path    = convertFromImage/task31722_0

   task31722_1.files = convertFromImage/task31722_1/*.png
   task31722_1.path    = convertFromImage/task31722_1
 
   icons.files = convertFromToHICON/*       
   icons.path = convertFromToHICON
   
   loadFromData.files = loadFromData/*
   loadFromData.path = loadFromData

   DEPLOYMENT += task31722_0 task31722_1 icons loadFromData
}

wince*: {
   DEFINES += SRCDIR=\\\".\\\"
   DEPLOYMENT_PLUGIN += qico
} else:symbian {
   LIBS += -lfbscli.dll -lbitgdi.dll -lgdi.dll
} else {
   DEFINES += SRCDIR=\\\"$$PWD\\\"
   win32:LIBS += -lgdi32 -luser32
}

RESOURCES += qpixmap.qrc

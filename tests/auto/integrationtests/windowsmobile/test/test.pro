
CONFIG += testcase
QT += widgets testlib
HEADERS += ddhelper.h
SOURCES += tst_windowsmobile.cpp ddhelper.cpp
RESOURCES += windowsmobile.qrc

TARGET = ../tst_windowsmobile

wincewm*: {
   addFiles.files = $$OUT_PWD/../testQMenuBar/*.exe
                

   addFiles.path = "\\Program Files\\tst_windowsmobile"
   DEPLOYMENT += addFiles
}

wincewm*: {
   LIBS += Ddraw.lib
}




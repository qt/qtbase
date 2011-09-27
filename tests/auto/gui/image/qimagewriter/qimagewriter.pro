load(qttest_p4)
QT += widgets
SOURCES += tst_qimagewriter.cpp
MOC_DIR=tmp
!contains(QT_CONFIG, no-tiff):DEFINES += QTEST_HAVE_TIFF
win32-msvc:QMAKE_CXXFLAGS -= -Zm200
win32-msvc:QMAKE_CXXFLAGS += -Zm800

wince*: {
   addFiles.files = images\\*.*
   addFiles.path = images
   DEPLOYMENT += addFiles
   DEFINES += SRCDIR=\\\".\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD\\\"
}

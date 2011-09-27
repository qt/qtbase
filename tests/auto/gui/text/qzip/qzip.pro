load(qttest_p4)
QT += gui-private
SOURCES += tst_qzip.cpp

wince* {
   addFiles.files = testdata
   addFiles.path = .
   DEPLOYMENT += addFiles
   DEFINES += SRCDIR=\\\".\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD\\\"
}

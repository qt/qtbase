CONFIG += testcase
TARGET = tst_qicon

QT += widgets testlib
SOURCES += tst_qicon.cpp
RESOURCES = tst_qicon.qrc

wince* {
   QT += xml svg
   addFiles.files += $$_PRO_FILE_PWD_/*.png
   addFiles.files += $$_PRO_FILE_PWD_/*.svg
   addFiles.files += $$_PRO_FILE_PWD_/*.svgz
   addFiles.files += $$_PRO_FILE_PWD_/tst_qicon.cpp
   addFiles.path = .
   DEPLOYMENT += addFiles

   DEPLOYMENT_PLUGIN += qsvg
   DEFINES += SRCDIR=\\\".\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD\\\"
}

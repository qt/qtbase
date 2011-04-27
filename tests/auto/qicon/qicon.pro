load(qttest_p4)

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
} else:symbian {
   QT += xml svg
   addFiles.files =  *.png tst_qicon.cpp *.svg *.svgz
   addFiles.path = .
   DEPLOYMENT += addFiles
   qt_not_deployed {
      plugins.files = qsvgicon.dll
      plugins.path = iconengines
      DEPLOYMENT += plugins
   }
} else {
   DEFINES += SRCDIR=\\\"$$PWD\\\"
}


load(qttest_p4)
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
} else:symbian {
   addFiles.files = images\\*.*
   addFiles.path = images
   DEPLOYMENT += addFiles
   qt_not_deployed {
      imagePlugins.files = qjpeg.dll qtiff.dll
      imagePlugins.path = imageformats
      DEPLOYMENT += imagePlugins
   }
} else {
   DEFINES += SRCDIR=\\\"$$PWD\\\"
}

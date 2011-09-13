load(qttest_p4)
QT += widgets
SOURCES += tst_qmovie.cpp
MOC_DIR=tmp

!contains(QT_CONFIG, no-gif):DEFINES += QTEST_HAVE_GIF
!contains(QT_CONFIG, no-jpeg):DEFINES += QTEST_HAVE_JPEG
!contains(QT_CONFIG, no-mng):DEFINES += QTEST_HAVE_MNG

wince*: {
   addFiles.files = animations\\*
   addFiles.path = animations
   DEPLOYMENT += addFiles
}

RESOURCES += resources.qrc

symbian: {
   addFiles.files = animations\\*
   addFiles.path = animations
   DEPLOYMENT += addFiles

   qt_not_deployed {
      imagePlugins.files = qjpeg.dll qgif.dll qmng.dll
      imagePlugins.path = imageformats
      DEPLOYMENT += imagePlugins
   }
}

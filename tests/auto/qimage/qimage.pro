load(qttest_p4)
SOURCES  += tst_qimage.cpp

QT += core-private gui-private

wince*: {
   addImages.files = images/*
   addImages.path = images
   DEPLOYMENT += addImages
   DEFINES += SRCDIR=\\\".\\\"
} else:symbian {
   TARGET.EPOCHEAPSIZE = 0x200000 0x800000
   addImages.files = images/*
   addImages.path = images
   DEPLOYMENT += addImages
   qt_not_deployed {
      imagePlugins.files = qjpeg.dll qgif.dll qmng.dll qtiff.dll qico.dll
      imagePlugins.path = imageformats
      DEPLOYMENT += imagePlugins
   }
} else {
   contains(QT_CONFIG, qt3support): QT += qt3support
   DEFINES += SRCDIR=\\\"$$PWD\\\"
}

load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_qimagereader

SOURCES += tst_qimagereader.cpp

!contains(QT_CONFIG, no-gif):DEFINES += QTEST_HAVE_GIF
!contains(QT_CONFIG, no-jpeg):DEFINES += QTEST_HAVE_JPEG
!contains(QT_CONFIG, no-mng):DEFINES += QTEST_HAVE_MNG
!contains(QT_CONFIG, no-tiff):DEFINES += QTEST_HAVE_TIFF
QT += network

wince*: {
   addFiles.files = images
   addFiles.path = .

   CONFIG(debug, debug|release):{
   imageFormatsPlugins.files = $$(QTDIR)/plugins/imageformats/*d4.dll
   }

   CONFIG(release, debug|release):{
   imageFormatsPlugins.files = $$(QTDIR)/plugins/imageformats/*[^d]4.dll
   }
   imageFormatsPlugins.path = imageformats
   DEPLOYMENT += addFiles imageFormatsPlugins
}


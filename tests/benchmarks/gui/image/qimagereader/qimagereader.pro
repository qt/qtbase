QT += testlib

TEMPLATE = app
TARGET = tst_bench_qimagereader

SOURCES += tst_qimagereader.cpp

!contains(QT_CONFIG, no-gif):DEFINES += QTEST_HAVE_GIF
!contains(QT_CONFIG, no-jpeg):DEFINES += QTEST_HAVE_JPEG
QT += network

wince*: {
   addFiles.files = images
   addFiles.path = .

   CONFIG(debug, debug|release):{
   imageFormatsPlugins.files = $$QT_BUILD_TREE/plugins/imageformats/*d4.dll
   }

   CONFIG(release, debug|release):{
   imageFormatsPlugins.files = $$QT_BUILD_TREE/plugins/imageformats/*[^d]4.dll
   }
   imageFormatsPlugins.path = imageformats
   DEPLOYMENT += addFiles imageFormatsPlugins
}

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

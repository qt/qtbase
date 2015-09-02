CONFIG += testcase
TARGET = tst_qicoimageformat
SOURCES+= tst_qicoimageformat.cpp
QT += testlib

wince {
   CONFIG(debug, debug|release):{
       addPlugins.files = $$QT_BUILD_TREE/plugins/imageformats/qico4d.dll
   } else {
       addPlugins.files = $$QT_BUILD_TREE/plugins/imageformats/qico4.dll
   }
   addPlugins.path = imageformats
   DEPLOYMENT += addPlugins
}
TESTDATA += icons/*
android:RESOURCES+=qicoimageformat.qrc

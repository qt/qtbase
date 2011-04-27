load(qttest_p4)
SOURCES+= tst_qicoimageformat.cpp

wince*: {
   DEFINES += SRCDIR=\\\".\\\"
   addFiles.files = icons
   addFiles.path = .
   CONFIG(debug, debug|release):{
       addPlugins.files = $$QT_BUILD_TREE/plugins/imageformats/qico4d.dll
   } else {
       addPlugins.files = $$QT_BUILD_TREE/plugins/imageformats/qico4.dll
   }
   addPlugins.path = imageformats
   DEPLOYMENT += addFiles addPlugins
} else:symbian {
   addFiles.files = icons
   addFiles.path = .
   DEPLOYMENT += addFiles
   qt_not_deployed {
      addPlugins.files = qico.dll
      addPlugins.path = imageformats
      DEPLOYMENT += addPlugins
   }
   TARGET.UID3 = 0xE0340004
   DEFINES += SYMBIAN_SRCDIR_UID=$$lower($$replace(TARGET.UID3,"0x",""))
} else {
   DEFINES += SRCDIR=\\\"$$PWD\\\"
}

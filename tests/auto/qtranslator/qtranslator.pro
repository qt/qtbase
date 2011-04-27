load(qttest_p4)
SOURCES  += tst_qtranslator.cpp
RESOURCES += qtranslator.qrc

wince*|symbian: {
   addFiles.files = hellotr_la.qm msgfmt_from_po.qm
   addFiles.path = .
   DEPLOYMENT += addFiles
}



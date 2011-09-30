load(qttest_p4)
QT += widgets
SOURCES  += tst_qtranslator.cpp
RESOURCES += qtranslator.qrc

wince* {
   addFiles.files = hellotr_la.qm msgfmt_from_po.qm
   addFiles.path = .
   DEPLOYMENT += addFiles
}

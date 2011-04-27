load(qttest_p4)
TEMPLATE = app
DEPENDPATH += parser
INCLUDEPATH += . parser 

# Input
HEADERS +=  parser/parser.h
SOURCES += tst_qxmlsimplereader.cpp parser/parser.cpp 

CONFIG += no_batch
QT += network xml
QT -= gui


wince*|symbian: {
   addFiles.files = encodings parser xmldocs
   addFiles.path = .
   DEPLOYMENT += addFiles
}

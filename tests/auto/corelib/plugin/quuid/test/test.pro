CONFIG += testcase

QT = core testlib
SOURCES += ../tst_quuid.cpp
TARGET = tst_quuid

CONFIG(debug_and_release_target) {
  CONFIG(debug, debug|release) {
    DESTDIR = ../debug
  } else {
    DESTDIR = ../release
  }
} else {
  DESTDIR = ..
}

wince* {
   addFile_processUniqueness.files = $$OUT_PWD/../testProcessUniqueness/testProcessUniqueness.exe 
   addFile_processUniqueness.path = testProcessUniqueness

   DEPLOYMENT += addFile_processUniqueness
}

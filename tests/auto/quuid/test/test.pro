load(qttest_p4)

QT = core
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

symbian {
   binDep.files = testProcessUniqueness.exe
   binDep.path = \\sys\\bin

   DEPLOYMENT += binDep
}

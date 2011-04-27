load(qttest_p4)
SOURCES += tst_qmainwindow.cpp

# Symbian toolchain does not support correct include semantics
symbian:INCPATH+=..\\..\\..\\include\\QtGui\\private


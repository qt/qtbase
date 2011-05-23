load(qttest_p4)
QT += widgets widgets-private
SOURCES += tst_qmainwindow.cpp
# Symbian toolchain does not support correct include semantics
symbian:INCPATH+=..\\..\\..\\include\\QtGui\\private


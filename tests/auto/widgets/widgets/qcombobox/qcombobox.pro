CONFIG += testcase
TARGET = tst_qcombobox
QT += widgets widgets-private gui-private core-private testlib testlib-private
DEFINES += QTEST_QPA_MOUSE_HANDLING
SOURCES  += tst_qcombobox.cpp

TESTDATA += qtlogo.png qtlogoinverted.png

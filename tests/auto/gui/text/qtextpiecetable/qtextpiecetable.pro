load(qttest_p4)
QT += widgets widgets-private
QT += core-private gui-private
SOURCES  += tst_qtextpiecetable.cpp
HEADERS += ../qtextdocument/common.h

requires(!win32)
requires(contains(QT_CONFIG,private_tests))


load(qttest_p4)
SOURCES  += tst_qtextpiecetable.cpp
HEADERS += ../qtextdocument/common.h

requires(!win32)
requires(contains(QT_CONFIG,private_tests))


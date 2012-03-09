CONFIG += testcase parallel_test
TARGET = tst_qtimeline
QT = core testlib
SOURCES = tst_qtimeline.cpp

win32:CONFIG+=insignificant_test # This has been blacklisted in the past

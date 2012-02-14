CONFIG += testcase parallel_test
TARGET = tst_qsettings
QT = core-private gui testlib
SOURCES = tst_qsettings.cpp
RESOURCES += qsettings.qrc

win32-msvc*:LIBS += advapi32.lib

win32: CONFIG += insignificant_test # QTBUG-24145

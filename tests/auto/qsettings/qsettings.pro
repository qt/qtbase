load(qttest_p4)

QT += core-private

SOURCES  += tst_qsettings.cpp
RESOURCES += qsettings.qrc

win32-msvc*:LIBS += advapi32.lib

CONFIG += parallel_test

load(qttest_p4)
SOURCES  += tst_qsettings.cpp
RESOURCES += qsettings.qrc

contains(QT_CONFIG, qt3support):QT += qt3support
CONFIG -= debug
CONFIG += release
win32-msvc*:LIBS += advapi32.lib

CONFIG += parallel_test

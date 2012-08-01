CONFIG += testcase
TARGET = tst_qsettings
QT = core-private gui testlib
SOURCES = tst_qsettings.cpp
RESOURCES += qsettings.qrc

win32-msvc*:LIBS += advapi32.lib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

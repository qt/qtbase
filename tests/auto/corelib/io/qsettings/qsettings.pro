CONFIG += testcase
TARGET = tst_qsettings
QT = core-private gui testlib
SOURCES = tst_qsettings.cpp
RESOURCES += qsettings.qrc
INCLUDEPATH += $$PWD/../../kernel/qmetatype

msvc: QMAKE_USE += advapi32
darwin: LIBS += -framework CoreFoundation

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

QT += widgets testlib
QT += core-private gui-private widgets-private

TEMPLATE = app
TARGET = tst_qtracebench

RESOURCES += qtracebench.qrc

SOURCES += tst_qtracebench.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

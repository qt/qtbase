TEMPLATE = app
TARGET = qtimer_vs_qmetaobject
INCLUDEPATH += .

CONFIG += release
#CONFIG += debug


SOURCES += tst_qtimer_vs_qmetaobject.cpp
QT = core testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

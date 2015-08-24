CONFIG += testcase
QT = core testlib dbus
TARGET = tst_qdbuscpp2xml

QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS

SOURCES += tst_qdbuscpp2xml.cpp \

RESOURCES += qdbuscpp2xml.qrc

HEADERS += test1.h

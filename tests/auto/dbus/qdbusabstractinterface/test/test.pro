CONFIG += testcase
SOURCES += ../tst_qdbusabstractinterface.cpp ../interface.cpp
HEADERS += ../interface.h

TARGET = ../tst_qdbusabstractinterface

QT = core testlib
QT += dbus
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

DBUS_INTERFACES = ../org.qtproject.QtDBus.Pinger.xml
QDBUSXML2CPP_INTERFACE_HEADER_FLAGS += -i ../interface.h

macx:CONFIG += insignificant_test # QTBUG-37469

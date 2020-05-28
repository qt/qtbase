#! [qmake_use]
QT += dbus
#! [qmake_use]
TEMPLATE = lib

TARGET = qtdbus_snippets

QT += core dbus xml
load(qt_common)

SOURCES += code/src_qdbus_qdbusabstractinterface.cpp \
           code/src_qdbus_qdbusinterface.cpp \
           code/src_qdbus_qdbuspendingcall.cpp \
           code/src_qdbus_qdbuspendingreply.cpp \
           code/src_qdbus_qdbusreply.cpp \
           code/doc_src_qdbusadaptors.cpp \
           code/src_qdbus_qdbusargument.cpp \
           code/src_qdbus_qdbuscontext.cpp \
           code/src_qdbus_qdbusmetatype.cpp

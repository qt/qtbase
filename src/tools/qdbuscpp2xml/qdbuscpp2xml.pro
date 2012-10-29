option(host_build)
TEMPLATE = app
TARGET = qdbuscpp2xml
QT = bootstrap-private

DESTDIR = ../../../bin

include(../moc/moc.pri)

DEFINES += QT_NO_CAST_FROM_ASCII

INCLUDEPATH += $$QT_BUILD_TREE/include \
                   $$QT_BUILD_TREE/include/QtDBus \
                   $$QT_BUILD_TREE/include/QtDBus/$$QT_VERSION \
                   $$QT_BUILD_TREE/include/QtDBus/$$QT_VERSION/QtDBus \
                   $$QT_SOURCE_TREE/src/dbus

!isEmpty(DBUS_PATH): INCLUDEPATH += $$DBUS_PATH/include

QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS

SOURCES += qdbuscpp2xml.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbusmetatype.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbusutil.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbusmisc.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbusargument.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbusmarshaller.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbusextratypes.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbus_symbols.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbusunixfiledescriptor.cpp

target.path = $$[QT_HOST_BINS]
INSTALLS += target
load(qt_targets)

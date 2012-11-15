option(host_build)

DEFINES += QT_NO_CAST_FROM_ASCII

INCLUDEPATH += $$QT_BUILD_TREE/include \
                   $$QT_BUILD_TREE/include/QtDBus \
                   $$QT_BUILD_TREE/include/QtDBus/$$QT_VERSION \
                   $$QT_BUILD_TREE/include/QtDBus/$$QT_VERSION/QtDBus \
                   $$QT_SOURCE_TREE/src/dbus

!isEmpty(DBUS_PATH): INCLUDEPATH += $$DBUS_PATH/include

QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS

SOURCES = qdbusxml2cpp.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbusintrospection.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbusxmlparser.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbuserror.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbusutil.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbusmetatype.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbusargument.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbusmarshaller.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbusextratypes.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbus_symbols.cpp \
          $$QT_SOURCE_TREE/src/dbus/qdbusunixfiledescriptor.cpp

load(qt_tool)

option(host_build)

MODULE = bootstrap_dbus
TARGET = QtBootstrapDBus
CONFIG += minimal_syncqt internal_module force_bootstrap

DEFINES += \
    QT_NO_CAST_FROM_ASCII

MODULE_INCNAME = QtDBus

load(qt_module)

QMAKE_CXXFLAGS += $$QT_HOST_CFLAGS_DBUS

SOURCES = \
    ../../dbus/qdbusintrospection.cpp \
    ../../dbus/qdbusxmlparser.cpp \
    ../../dbus/qdbuserror.cpp \
    ../../dbus/qdbusutil.cpp \
    ../../dbus/qdbusmisc.cpp \
    ../../dbus/qdbusmetatype.cpp \
    ../../dbus/qdbusargument.cpp \
    ../../dbus/qdbusextratypes.cpp \
    ../../dbus/qdbus_symbols.cpp \
    ../../dbus/qdbusunixfiledescriptor.cpp

lib.CONFIG = dummy_install
INSTALLS = lib

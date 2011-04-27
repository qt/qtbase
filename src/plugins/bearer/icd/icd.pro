TARGET = qicdbearer
include(../../qpluginbase.pri)

QT = core network dbus

QMAKE_CXXFLAGS *= $$QT_CFLAGS_DBUS $$QT_CFLAGS_CONNSETTINGS
LIBS += $$QT_LIBS_CONNSETTINGS

HEADERS += qicdengine.h \
           qnetworksession_impl.h \
           dbusdispatcher.h \
           iapconf.h \
           iapmonitor.h \
           maemo_icd.h \
           proxyconf.h \
           wlan-utils.h

SOURCES += main.cpp \
           qicdengine.cpp \
           qnetworksession_impl.cpp \
           dbusdispatcher.cpp \
           iapmonitor.cpp \
           iapconf.cpp \
           maemo_icd.cpp \
           proxyconf.cpp

#DEFINES += BEARER_MANAGEMENT_DEBUG

include(../../../3rdparty/libgq.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/bearer
target.path += $$[QT_INSTALL_PLUGINS]/bearer
INSTALLS += target

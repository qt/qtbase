TARGET = qcorewlanbearer
load(qt_plugin)

QT = core-private network-private
LIBS += -framework Foundation -framework SystemConfiguration

contains(QT_CONFIG, corewlan) {
    isEmpty(QMAKE_MAC_SDK)|contains(QMAKE_MAC_SDK, "/Developer/SDKs/MacOSX10.6.sdk") {
         LIBS += -framework CoreWLAN -framework Security
         DEFINES += MAC_SDK_10_6
    }
}

HEADERS += qcorewlanengine.h \
           ../qnetworksession_impl.h \
           ../qbearerengine_impl.h

SOURCES += main.cpp \
           ../qnetworksession_impl.cpp

OBJECTIVE_SOURCES += qcorewlanengine.mm

DESTDIR = $$QT.network.plugins/bearer
target.path += $$[QT_INSTALL_PLUGINS]/bearer
INSTALLS += target

TARGET = qsymbianbearer
include(../../qpluginbase.pri)

QT = core network

HEADERS += ../symbianengine.h \
           ../qnetworksession_impl.h

SOURCES += ../symbianengine.cpp \
           ../qnetworksession_impl.cpp \
           ../main.cpp

INCLUDEPATH += $$APP_LAYER_SYSTEMINCLUDE
symbian-abld:INCLUDEPATH += $$QT_BUILD_TREE/include/QtNetwork/private

LIBS += -lcommdb \
        -lcentralrepository \
        -lesock \
        -linsock \
        -lecom \
        -lefsrv \
        -lnetmeta

is_using_gnupoc {
    LIBS += -lconnmon
} else {
    LIBS += -lConnMon
}

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/bearer
target.path += $$[QT_INSTALL_PLUGINS]/bearer
INSTALLS += target

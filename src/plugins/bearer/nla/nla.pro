TARGET = qnlabearer
include(../../qpluginbase.pri)

QT = core network

!wince* {
    LIBS += -lWs2_32
} else {
    LIBS += -lWs2
}

HEADERS += qnlaengine.h \
           ../platformdefs_win.h \
           ../qnetworksession_impl.h \
           ../qbearerengine_impl.h

SOURCES += main.cpp \
           qnlaengine.cpp \
           ../qnetworksession_impl.cpp

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/bearer
target.path += $$[QT_INSTALL_PLUGINS]/bearer
INSTALLS += target

TARGET = blackberry
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms
QT += opengl opengl-private platformsupport platformsupport-private widgets-private

# Uncomment this to build with support for IMF once it becomes available in the BBNDK
#CONFIG += qbb_imf

# Uncomment these to enable debugging output for various aspects of the plugin
#DEFINES += QBBBUFFER_DEBUG
#DEFINES += QBBCLIPBOARD_DEBUG
#DEFINES += QBBEVENTTHREAD_DEBUG
#DEFINES += QBBGLBACKINGSTORE_DEBUG
#DEFINES += QBBGLCONTEXT_DEBUG
#DEFINES += QBBINPUTCONTEXT_DEBUG
#DEFINES += QBBINPUTCONTEXT_IMF_EVENT_DEBUG
#DEFINES += QBBINTEGRATION_DEBUG
#DEFINES += QBBNAVIGATORTHREAD_DEBUG
#DEFINES += QBBRASTERBACKINGSTORE_DEBUG
#DEFINES += QBBROOTWINDOW_DEBUG
#DEFINES += QBBSCREEN_DEBUG
#DEFINES += QBBVIRTUALKEYBOARD_DEBUG
#DEFINES += QBBWINDOW_DEBUG

SOURCES =   main.cpp \
            qbbbuffer.cpp \
            qbbeventthread.cpp \
            qbbglcontext.cpp \
            qbbglbackingstore.cpp \
            qbbintegration.cpp \
            qbbnavigatorthread.cpp \
            qbbscreen.cpp \
            qbbwindow.cpp \
            qbbrasterbackingstore.cpp \
            qbbvirtualkeyboard.cpp \
            qbbclipboard.cpp \
            qbbrootwindow.cpp

HEADERS =   qbbbuffer.h \
            qbbeventthread.h \
            qbbkeytranslator.h \
            qbbintegration.h \
            qbbnavigatorthread.h \
            qbbglcontext.h \
            qbbglbackingstore.h \
            qbbscreen.h \
            qbbwindow.h \
            qbbrasterbackingstore.h \
            qbbvirtualkeyboard.h \
            qbbclipboard.h \
            qbbrootwindow.h

CONFIG(qbb_imf) {
    DEFINES += QBB_IMF
    HEADERS += qbbinputcontext_imf.h
    SOURCES += qbbinputcontext_imf.cpp
} else {
    HEADERS += qbbinputcontext_noimf.h
    SOURCES += qbbinputcontext_noimf.cpp
}

QMAKE_CXXFLAGS += -I./private

LIBS += -lpps -lscreen -lEGL -lclipboard

include (../../../platformsupport/eglconvenience/eglconvenience.pri)
include (../../../platformsupport/fontdatabases/fontdatabases.pri)

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

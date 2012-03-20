TARGET = qnx
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms
QT += opengl opengl-private platformsupport platformsupport-private widgets-private

# Uncomment this to build with support for IMF once it becomes available in the BBNDK
#CONFIG += qqnx_imf

# Uncomment these to enable debugging output for various aspects of the plugin
#DEFINES += QQNXBUFFER_DEBUG
#DEFINES += QQNXCLIPBOARD_DEBUG
#DEFINES += QQNXEVENTTHREAD_DEBUG
#DEFINES += QQNXGLBACKINGSTORE_DEBUG
#DEFINES += QQNXGLCONTEXT_DEBUG
#DEFINES += QQNXINPUTCONTEXT_DEBUG
#DEFINES += QQNXINPUTCONTEXT_IMF_EVENT_DEBUG
#DEFINES += QQNXINTEGRATION_DEBUG
#DEFINES += QQNXNAVIGATOREVENTHANDLER_DEBUG
#DEFINES += QQNXRASTERBACKINGSTORE_DEBUG
#DEFINES += QQNXROOTWINDOW_DEBUG
#DEFINES += QQNXSCREEN_DEBUG
#DEFINES += QQNXVIRTUALKEYBOARD_DEBUG
#DEFINES += QQNXWINDOW_DEBUG

SOURCES =   main.cpp \
            qqnxbuffer.cpp \
            qqnxeventthread.cpp \
            qqnxglcontext.cpp \
            qqnxglbackingstore.cpp \
            qqnxintegration.cpp \
            qqnxnavigatoreventhandler.cpp \
            qqnxscreen.cpp \
            qqnxwindow.cpp \
            qqnxrasterbackingstore.cpp \
            qqnxvirtualkeyboard.cpp \
            qqnxclipboard.cpp \
            qqnxrootwindow.cpp

HEADERS =   qqnxbuffer.h \
            qqnxeventthread.h \
            qqnxkeytranslator.h \
            qqnxintegration.h \
            qqnxnavigatoreventhandler.h \
            qqnxglcontext.h \
            qqnxglbackingstore.h \
            qqnxscreen.h \
            qqnxwindow.h \
            qqnxrasterbackingstore.h \
            qqnxvirtualkeyboard.h \
            qqnxclipboard.h \
            qqnxrootwindow.h

CONFIG(qqnx_imf) {
    DEFINES += QQNX_IMF
    HEADERS += qqnxinputcontext_imf.h
    SOURCES += qqnxinputcontext_imf.cpp
} else {
    HEADERS += qqnxinputcontext_noimf.h
    SOURCES += qqnxinputcontext_noimf.cpp
}

QMAKE_CXXFLAGS += -I./private

LIBS += -lpps -lscreen -lEGL -lclipboard

include (../../../platformsupport/eglconvenience/eglconvenience.pri)
include (../../../platformsupport/fontdatabases/fontdatabases.pri)

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

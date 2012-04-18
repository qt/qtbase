TARGET = qnx
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms
QT += platformsupport platformsupport-private widgets-private

contains(QT_CONFIG, opengles2) {
    QT += opengl opengl-private
}

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
#DEFINES += QQNXNAVIGATOREVENTNOTIFIER_DEBUG
#DEFINES += QQNXRASTERBACKINGSTORE_DEBUG
#DEFINES += QQNXROOTWINDOW_DEBUG
#DEFINES += QQNXSCREEN_DEBUG
#DEFINES += QQNXSCREENEVENT_DEBUG
#DEFINES += QQNXVIRTUALKEYBOARD_DEBUG
#DEFINES += QQNXWINDOW_DEBUG

SOURCES =   main.cpp \
            qqnxbuffer.cpp \
            qqnxeventthread.cpp \
            qqnxintegration.cpp \
            qqnxscreen.cpp \
            qqnxwindow.cpp \
            qqnxrasterbackingstore.cpp \
            qqnxrootwindow.cpp \
            qqnxscreeneventhandler.cpp \
            qqnxnativeinterface.cpp

CONFIG(blackberry) {
    SOURCES += qqnxnavigatoreventhandler.cpp \
               qqnxnavigatoreventnotifier.cpp \
               qqnxvirtualkeyboard.cpp \
               qqnxclipboard.cpp \
               qqnxabstractvirtualkeyboard.cpp
}

contains(QT_CONFIG, opengles2) {
    SOURCES += qqnxglcontext.cpp \
               qqnxglbackingstore.cpp
}

HEADERS =   main.h \
            qqnxbuffer.h \
            qqnxeventthread.h \
            qqnxkeytranslator.h \
            qqnxintegration.h \
            qqnxscreen.h \
            qqnxwindow.h \
            qqnxrasterbackingstore.h \
            qqnxrootwindow.h \
            qqnxscreeneventhandler.h \
            qqnxnativeinterface.h

CONFIG(blackberry) {
    HEADERS += qqnxnavigatoreventhandler.h \
               qqnxnavigatoreventnotifier.h \
               qqnxvirtualkeyboard.h \
               qqnxclipboard.h \
               qqnxabstractvirtualkeyboard.h
}

contains(QT_CONFIG, opengles2) {
    HEADERS += qqnxglcontext.h \
               qqnxglbackingstore.h
}


CONFIG(blackberry) {
    SOURCES += qqnxservices.cpp
    HEADERS += qqnxservices.h

    CONFIG(qqnx_imf) {
        DEFINES += QQNX_IMF
        HEADERS += qqnxinputcontext_imf.h
        SOURCES += qqnxinputcontext_imf.cpp
    } else {
        HEADERS += qqnxinputcontext_noimf.h
        SOURCES += qqnxinputcontext_noimf.cpp
    }
}

OTHER_FILES += qnx.json

QMAKE_CXXFLAGS += -I./private

LIBS += -lscreen

contains(QT_CONFIG, opengles2) {
    LIBS += -lEGL
}

CONFIG(blackberry) {
    LIBS += -lbps -lpps -lclipboard
}

include (../../../platformsupport/eglconvenience/eglconvenience.pri)
include (../../../platformsupport/fontdatabases/fontdatabases.pri)

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

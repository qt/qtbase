TARGET = qqnx

QT += \
    core-private gui-private \
    fontdatabase_support-private eventdispatcher_support-private egl_support-private

# Uncomment this to build with support for IMF once it becomes available in the BBNDK
#CONFIG += qqnx_imf

# Uncomment these to enable debugging output for various aspects of the plugin
#DEFINES += QQNXBUFFER_DEBUG
#DEFINES += QQNXBUTTON_DEBUG
#DEFINES += QQNXCLIPBOARD_DEBUG
#DEFINES += QQNXFILEDIALOGHELPER_DEBUG
#DEFINES += QQNXGLBACKINGSTORE_DEBUG
#DEFINES += QQNXGLCONTEXT_DEBUG
#DEFINES += QQNXINPUTCONTEXT_DEBUG
#DEFINES += QQNXINPUTCONTEXT_IMF_EVENT_DEBUG
#DEFINES += QQNXINTEGRATION_DEBUG
#DEFINES += QQNXNAVIGATOREVENTHANDLER_DEBUG
#DEFINES += QQNXNAVIGATOREVENTNOTIFIER_DEBUG
#DEFINES += QQNXNAVIGATOR_DEBUG
#DEFINES += QQNXRASTERBACKINGSTORE_DEBUG
#DEFINES += QQNXSCREENEVENTTHREAD_DEBUG
#DEFINES += QQNXSCREENEVENT_DEBUG
#DEFINES += QQNXSCREEN_DEBUG
#DEFINES += QQNXVIRTUALKEYBOARD_DEBUG
#DEFINES += QQNXWINDOW_DEBUG
#DEFINES += QQNXCURSOR_DEBUG
#DEFINES += QQNXFILEPICKER_DEBUG
#DEFINES += QQNXEGLWINDOW_DEBUG
#DEFINES += QQNXRASTERWINDOW_DEBUG

SOURCES =   main.cpp \
            qqnxbuffer.cpp \
            qqnxforeignwindow.cpp \
            qqnxintegration.cpp \
            qqnxscreen.cpp \
            qqnxwindow.cpp \
            qqnxrasterbackingstore.cpp \
            qqnxscreeneventhandler.cpp \
            qqnxnativeinterface.cpp \
            qqnxnavigatoreventhandler.cpp \
            qqnxabstractnavigator.cpp \
            qqnxabstractvirtualkeyboard.cpp \
            qqnxservices.cpp \
            qqnxcursor.cpp \
            qqnxrasterwindow.cpp \
            qqnxglobal.cpp \
            qqnxscreeneventthread.cpp

HEADERS =   main.h \
            qqnxbuffer.h \
            qqnxforeignwindow.h \
            qqnxkeytranslator.h \
            qqnxintegration.h \
            qqnxscreen.h \
            qqnxwindow.h \
            qqnxrasterbackingstore.h \
            qqnxscreeneventhandler.h \
            qqnxnativeinterface.h \
            qqnxnavigatoreventhandler.h \
            qqnxabstractnavigator.h \
            qqnxabstractvirtualkeyboard.h \
            qqnxabstractcover.h \
            qqnxservices.h \
            qqnxcursor.h \
            qqnxrasterwindow.h \
            qqnxscreeneventfilter.h \
            qqnxglobal.h \
            qqnxlgmon.h \
            qqnxscreeneventthread.h

LIBS += -lscreen

qtConfig(egl) {
    SOURCES += qqnxglcontext.cpp \
               qqnxeglwindow.cpp

    HEADERS += qqnxglcontext.h \
               qqnxeglwindow.h

    QMAKE_USE += egl
}

qtConfig(qqnx_pps) {
    SOURCES += qqnxbuttoneventnotifier.cpp \
               qqnxnavigatorpps.cpp \
               qqnxnavigatoreventnotifier.cpp \
               qqnxvirtualkeyboardpps.cpp

    HEADERS += qqnxbuttoneventnotifier.h \
               qqnxnavigatorpps.h \
               qqnxnavigatoreventnotifier.h \
               qqnxvirtualkeyboardpps.h

    QMAKE_USE += pps

    qtConfig(clipboard) {
        SOURCES += qqnxclipboard.cpp
        HEADERS += qqnxclipboard.h
        LIBS += -lclipboard
    }

    qtConfig(qqnx_imf) {
        HEADERS += qqnxinputcontext_imf.h
        SOURCES += qqnxinputcontext_imf.cpp
    } else {
        HEADERS += qqnxinputcontext_noimf.h
        SOURCES += qqnxinputcontext_noimf.cpp
    }
}

lgmon {
    DEFINES += QQNX_LGMON
    SOURCES += qqnxlgmon.cpp
    QMAKE_USE += lgmon
}

OTHER_FILES += qnx.json

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QQnxIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

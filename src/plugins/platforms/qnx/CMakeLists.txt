# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QQnxIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QQnxIntegrationPlugin
    OUTPUT_NAME qqnx
    PLUGIN_TYPE platforms
    DEFAULT_IF "qnx" IN_LIST QT_QPA_PLATFORMS
    SOURCES
        main.cpp main.h
        qqnxabstractcover.h
        qqnxabstractnavigator.cpp qqnxabstractnavigator.h
        qqnxabstractvirtualkeyboard.cpp qqnxabstractvirtualkeyboard.h
        qqnxbuffer.cpp qqnxbuffer.h
        qqnxcursor.cpp qqnxcursor.h
        qqnxforeignwindow.cpp qqnxforeignwindow.h
        qqnxglobal.cpp qqnxglobal.h
        qqnxintegration.cpp qqnxintegration.h
        qqnxkeytranslator.h
        qqnxlgmon.h
        qqnxnativeinterface.cpp qqnxnativeinterface.h
        qqnxnavigatoreventhandler.cpp qqnxnavigatoreventhandler.h
        qqnxrasterbackingstore.cpp qqnxrasterbackingstore.h
        qqnxrasterwindow.cpp qqnxrasterwindow.h
        qqnxscreen.cpp qqnxscreen.h
        qqnxscreeneventfilter.h
        qqnxscreeneventhandler.cpp qqnxscreeneventhandler.h
        qqnxscreeneventthread.cpp qqnxscreeneventthread.h
        qqnxservices.cpp qqnxservices.h
        qqnxwindow.cpp qqnxwindow.h
    NO_PCH_SOURCES
        qqnxclipboard.cpp # undef QT_NO_FOREACH
        qqnxintegration.cpp # undef QT_NO_FOREACH
        qqnxscreen.cpp # undef QT_NO_FOREACH
        qqnxscreeneventhandler.cpp # undef QT_NO_FOREACH
        qqnxwindow.cpp # undef QT_NO_FOREACH
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        screen
)

## Scopes:
#####################################################################

qt_internal_extend_target(QQnxIntegrationPlugin CONDITION QT_FEATURE_egl
    SOURCES
        qqnxeglwindow.cpp qqnxeglwindow.h
        qqnxglcontext.cpp qqnxglcontext.h
    LIBRARIES
        EGL::EGL
)

qt_internal_extend_target(QQnxIntegrationPlugin CONDITION QT_FEATURE_qqnx_pps
    SOURCES
        qqnxbuttoneventnotifier.cpp qqnxbuttoneventnotifier.h
        qqnxnavigatoreventnotifier.cpp qqnxnavigatoreventnotifier.h
        qqnxnavigatorpps.cpp qqnxnavigatorpps.h
        qqnxvirtualkeyboardpps.cpp qqnxvirtualkeyboardpps.h
    LIBRARIES
        PPS::PPS
)

qt_internal_extend_target(QQnxIntegrationPlugin CONDITION QT_FEATURE_clipboard AND QT_FEATURE_qqnx_pps
    SOURCES
        qqnxclipboard.cpp qqnxclipboard.h
    LIBRARIES
        clipboard
)

qt_internal_extend_target(QQnxIntegrationPlugin CONDITION QT_FEATURE_qqnx_imf AND QT_FEATURE_qqnx_pps
    SOURCES
        qqnxinputcontext_imf.cpp qqnxinputcontext_imf.h
)

qt_internal_extend_target(QQnxIntegrationPlugin CONDITION QT_FEATURE_qqnx_pps AND NOT QT_FEATURE_qqnx_imf
    SOURCES
        qqnxinputcontext_noimf.cpp qqnxinputcontext_noimf.h
)

qt_internal_extend_target(QQnxIntegrationPlugin CONDITION lgmon
    SOURCES
        qqnxlgmon.cpp
    DEFINES
        QQNX_LGMON
    LIBRARIES
        lgmon
)

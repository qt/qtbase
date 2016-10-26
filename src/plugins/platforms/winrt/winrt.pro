TARGET = qwinrt

CONFIG -= precompile_header

QT += \
    core-private gui-private \
    fontdatabase_support-private egl_support-private

DEFINES *= QT_NO_CAST_FROM_ASCII __WRL_NO_DEFAULT_LIB__

LIBS += $$QMAKE_LIBS_CORE -ld3d11

SOURCES = \
    main.cpp  \
    qwinrtbackingstore.cpp \
    qwinrtclipboard.cpp \
    qwinrtcursor.cpp \
    qwinrtdrag.cpp \
    qwinrteglcontext.cpp \
    qwinrteventdispatcher.cpp \
    qwinrtfiledialoghelper.cpp \
    qwinrtfileengine.cpp \
    qwinrtinputcontext.cpp \
    qwinrtintegration.cpp \
    qwinrtmessagedialoghelper.cpp \
    qwinrtscreen.cpp \
    qwinrtservices.cpp \
    qwinrttheme.cpp \
    qwinrtwindow.cpp


HEADERS = \
    qwinrtbackingstore.h \
    qwinrtclipboard.h \
    qwinrtcursor.h \
    qwinrtdrag.h \
    qwinrteglcontext.h \
    qwinrteventdispatcher.h \
    qwinrtfiledialoghelper.h \
    qwinrtfileengine.h \
    qwinrtinputcontext.h \
    qwinrtintegration.h \
    qwinrtmessagedialoghelper.h \
    qwinrtscreen.h \
    qwinrtservices.h \
    qwinrttheme.h \
    qwinrtwindow.h

OTHER_FILES += winrt.json

WINRT_SDK_VERSION_STRING = $$(UCRTVersion)
WINRT_SDK_VERSION = $$member($$list($$split(WINRT_SDK_VERSION_STRING, .)), 2)
lessThan(WINRT_SDK_VERSION, 14322): DEFINES += QT_WINRT_LIMITED_DRAGANDDROP

*-msvc2013|contains(DEFINES, QT_NO_DRAGANDDROP) {
    SOURCES -= qwinrtdrag.cpp
    HEADERS -= qwinrtdrag.h
}

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QWinRTIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

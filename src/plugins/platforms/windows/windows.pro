TARGET = windows
load(qt_plugin)

QT *= core-private
QT *= gui-private

INCLUDEPATH += ../../../3rdparty/harfbuzz/src
QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms

# Note: OpenGL32 must precede Gdi32 as it overwrites some functions.
LIBS *= -lOpenGL32 -lGdi32 -lUser32 -lOle32 -lWinspool -lImm32 -lWinmm  -lOleaut32
win32-g++: LIBS *= -luuid

contains(QT_CONFIG, directwrite) {
    LIBS *= -ldwrite
    SOURCES += qwindowsfontenginedirectwrite.cpp
    HEADERS += qwindowsfontenginedirectwrite.h
} else {
    DEFINES *= QT_NO_DIRECTWRITE
}

SOURCES += \
    main.cpp \
    qwindowsnativeimage.cpp \
    qwindowswindow.cpp \
    qwindowsintegration.cpp \
    qwindowscontext.cpp \
    qwindowsbackingstore.cpp \
    qwindowsscreen.cpp \
    qwindowskeymapper.cpp \
    qwindowsfontengine.cpp \
    qwindowsfontdatabase.cpp \
    qwindowsmousehandler.cpp \
    qwindowsguieventdispatcher.cpp \
    qwindowsglcontext.cpp \
    qwindowsclipboard.cpp \
    qwindowsole.cpp \
    qwindowsmime.cpp \
    qwindowsdrag.cpp \
    qwindowscursor.cpp \
    pixmaputils.cpp \
    qwindowsinputcontext.cpp \
    qwindowsaccessibility.cpp

HEADERS += \
    qwindowsnativeimage.h \
    qwindowswindow.h \
    qwindowsintegration.h \
    qwindowscontext.h \
    qwindowsbackingstore.h \
    qwindowsscreen.h \
    qwindowskeymapper.h \
    qwindowsfontengine.h \
    qwindowsfontdatabase.h \
    qwindowsmousehandler.h \
    qwindowsguieventdispatcher.h \
    qtwindowsglobal.h \
    qtwindows_additional.h \
    qwindowsglcontext.h \
    qwindowsclipboard.h \
    qwindowsole.h \
    qwindowsmime.h \
    qwindowsdrag.h \
    qwindowsinternalmimedata.h \
    qwindowscursor.h \
    pixmaputils.h \
    array.h \
    qwindowsinputcontext.h \
    qwindowsaccessibility.h

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

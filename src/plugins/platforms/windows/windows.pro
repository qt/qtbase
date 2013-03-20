TARGET = qwindows

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QWindowsIntegrationPlugin
load(qt_plugin)

QT *= core-private
QT *= gui-private
QT *= platformsupport-private

# Note: OpenGL32 must precede Gdi32 as it overwrites some functions.
LIBS *= -lole32
!wince*:LIBS *= -lgdi32 -luser32 -lwinspool -limm32 -lwinmm  -loleaut32

contains(QT_CONFIG, opengl):!contains(QT_CONFIG, opengles2):LIBS *= -lopengl32

win32-g++*: LIBS *= -luuid
# For the dialog helpers:
!wince*:LIBS *= -lshlwapi -lshell32
!wince*:LIBS *= -ladvapi32
wince*:DEFINES *= QT_LIBINFIX=L"\"\\\"$${QT_LIBINFIX}\\\"\""

DEFINES *= QT_NO_CAST_FROM_ASCII

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
    qwindowsole.cpp \
    qwindowsmime.cpp \
    qwindowsinternalmimedata.cpp \
    qwindowscursor.cpp \
    qwindowsinputcontext.cpp \
    qwindowstheme.cpp \
    qwindowsdialoghelpers.cpp \
    qwindowsservices.cpp

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
    qwindowsole.h \
    qwindowsmime.h \
    qwindowsinternalmimedata.h \
    qwindowscursor.h \
    array.h \
    qwindowsinputcontext.h \
    qwindowstheme.h \
    qwindowsdialoghelpers.h \
    qwindowsservices.h \
    qplatformfunctions_wince.h

contains(QT_CONFIG, opengles2) {
    SOURCES += qwindowseglcontext.cpp
    HEADERS += qwindowseglcontext.h
} else {
    contains(QT_CONFIG, opengl) {
        SOURCES += qwindowsglcontext.cpp
        HEADERS += qwindowsglcontext.h
   }
}

!contains( DEFINES, QT_NO_CLIPBOARD ) {
    SOURCES += qwindowsclipboard.cpp
    HEADERS += qwindowsclipboard.h
}

# drag and drop on windows only works if a clipboard is available
!contains( DEFINES, QT_NO_DRAGANDDROP ) {
    !win32:SOURCES += qwindowsdrag.cpp
    !win32:HEADERS += qwindowsdrag.h
    win32:!contains( DEFINES, QT_NO_CLIPBOARD ) {
        HEADERS += qwindowsdrag.h
        SOURCES += qwindowsdrag.cpp
    }
}

contains(QT_CONFIG, freetype) {
    DEFINES *= QT_NO_FONTCONFIG
    QT_FREETYPE_DIR = $$QT_SOURCE_TREE/src/3rdparty/freetype

    HEADERS += \
               qwindowsfontdatabase_ft.h
    SOURCES += \
               qwindowsfontdatabase_ft.cpp \
               $$QT_FREETYPE_DIR/src/base/ftbase.c \
               $$QT_FREETYPE_DIR/src/base/ftbbox.c \
               $$QT_FREETYPE_DIR/src/base/ftdebug.c \
               $$QT_FREETYPE_DIR/src/base/ftglyph.c \
               $$QT_FREETYPE_DIR/src/base/ftinit.c \
               $$QT_FREETYPE_DIR/src/base/ftmm.c \
               $$QT_FREETYPE_DIR/src/base/fttype1.c \
               $$QT_FREETYPE_DIR/src/base/ftsynth.c \
               $$QT_FREETYPE_DIR/src/base/ftbitmap.c \
               $$QT_FREETYPE_DIR/src/bdf/bdf.c \
               $$QT_FREETYPE_DIR/src/cache/ftcache.c \
               $$QT_FREETYPE_DIR/src/cff/cff.c \
               $$QT_FREETYPE_DIR/src/cid/type1cid.c \
               $$QT_FREETYPE_DIR/src/gzip/ftgzip.c \
               $$QT_FREETYPE_DIR/src/pcf/pcf.c \
               $$QT_FREETYPE_DIR/src/pfr/pfr.c \
               $$QT_FREETYPE_DIR/src/psaux/psaux.c \
               $$QT_FREETYPE_DIR/src/pshinter/pshinter.c \
               $$QT_FREETYPE_DIR/src/psnames/psmodule.c \
               $$QT_FREETYPE_DIR/src/raster/raster.c \
               $$QT_FREETYPE_DIR/src/sfnt/sfnt.c \
               $$QT_FREETYPE_DIR/src/smooth/smooth.c \
               $$QT_FREETYPE_DIR/src/truetype/truetype.c \
               $$QT_FREETYPE_DIR/src/type1/type1.c \
               $$QT_FREETYPE_DIR/src/type42/type42.c \
               $$QT_FREETYPE_DIR/src/winfonts/winfnt.c \
               $$QT_FREETYPE_DIR/src/lzw/ftlzw.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvalid.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvbase.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvgdef.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvjstf.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvcommn.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvgpos.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvgsub.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvmod.c\
               $$QT_FREETYPE_DIR/src/autofit/afangles.c\
               $$QT_FREETYPE_DIR/src/autofit/afglobal.c\
               $$QT_FREETYPE_DIR/src/autofit/aflatin.c\
               $$QT_FREETYPE_DIR/src/autofit/afmodule.c\
               $$QT_FREETYPE_DIR/src/autofit/afdummy.c\
               $$QT_FREETYPE_DIR/src/autofit/afhints.c\
               $$QT_FREETYPE_DIR/src/autofit/afloader.c\
               $$QT_FREETYPE_DIR/src/autofit/autofit.c

   SOURCES += $$QT_FREETYPE_DIR/src/base/ftsystem.c


   INCLUDEPATH += \
       $$QT_FREETYPE_DIR/src \
       $$QT_FREETYPE_DIR/include

   TR_EXCLUDE += $$QT_FREETYPE_DIR/*

   DEFINES += FT2_BUILD_LIBRARY
   contains(QT_CONFIG, system-zlib) {
        DEFINES += FT_CONFIG_OPTION_SYSTEM_ZLIB
   }
} else:contains(QT_CONFIG, system-freetype) {
    include($$QT_SOURCE_TREE/src/platformsupport/fontdatabases/basic/basic.pri)
    HEADERS += \
               qwindowsfontdatabase_ft.h
    SOURCES += \
               qwindowsfontdatabase_ft.cpp
}

OTHER_FILES += windows.json

contains(QT_CONFIG, accessibility):include(accessible/accessible.pri)

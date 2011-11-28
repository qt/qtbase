TARGET = windows
load(qt_plugin)

QT *= core-private
QT *= gui-private
QT *= platformsupport-private
# ### fixme: Remove widgets dependencies of dialog helpers
QT *= widgets

INCLUDEPATH += ../../../3rdparty/harfbuzz/src
QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms

# Note: OpenGL32 must precede Gdi32 as it overwrites some functions.
LIBS *= -lOpenGL32 -lGdi32 -lUser32 -lOle32 -lWinspool -lImm32 -lWinmm  -lOleaut32
win32-g++: LIBS *= -luuid
# For the dialog helpers:
LIBS *= -lshlwapi -lShell32

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
    qwindowsinputcontext.cpp \
    qwindowsaccessibility.cpp \
    qwindowstheme.cpp \
    qwindowsdialoghelpers.cpp

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
    array.h \
    qwindowsinputcontext.h \
    qwindowsaccessibility.h \
    qwindowstheme.h \
    qwindowsdialoghelpers.h

contains(QT_CONFIG, freetype) {
    DEFINES *= QT_NO_FONTCONFIG
    DEFINES *= QT_COMPILES_IN_HARFBUZZ
    QT_FREETYPE_DIR = $$QT_SOURCE_TREE/src/3rdparty/freetype

    HEADERS += \
               qwindowsfontdatabase_ft.h \
               $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft_p.h
    SOURCES += \
               qwindowsfontdatabase_ft.cpp \
               $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft.cpp \
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

   DEFINES += FT2_BUILD_LIBRARY
   contains(QT_CONFIG, system-zlib) {
        DEFINES += FT_CONFIG_OPTION_SYSTEM_ZLIB
   }
}

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

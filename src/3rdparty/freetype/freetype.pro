TARGET = qtfreetype

CONFIG += \
    static \
    hide_symbols \
    exceptions_off rtti_off warn_off \
    installed

MODULE_INCLUDEPATH += $$PWD/include

load(qt_helper_lib)

SOURCES += \
    $$PWD/src/autofit/afangles.c \
    $$PWD/src/autofit/afdummy.c \
    $$PWD/src/autofit/afglobal.c \
    $$PWD/src/autofit/afhints.c \
    $$PWD/src/autofit/aflatin.c \
    $$PWD/src/autofit/afloader.c \
    $$PWD/src/autofit/afmodule.c \
    $$PWD/src/autofit/autofit.c \
    $$PWD/src/base/ftbase.c \
    $$PWD/src/base/ftbitmap.c \
    $$PWD/src/base/ftbbox.c \
    $$PWD/src/base/ftdebug.c \
    $$PWD/src/base/ftglyph.c \
    $$PWD/src/base/ftfntfmt.c \
    $$PWD/src/base/ftinit.c \
    $$PWD/src/base/ftlcdfil.c \
    $$PWD/src/base/ftmm.c \
    $$PWD/src/base/ftsynth.c \
    $$PWD/src/base/fttype1.c \
    $$PWD/src/bdf/bdf.c \
    $$PWD/src/cache/ftcache.c \
    $$PWD/src/cff/cff.c \
    $$PWD/src/cid/type1cid.c \
    $$PWD/src/gzip/ftgzip.c \
    $$PWD/src/lzw/ftlzw.c \
    $$PWD/src/otvalid/otvalid.c \
    $$PWD/src/otvalid/otvbase.c \
    $$PWD/src/otvalid/otvcommn.c \
    $$PWD/src/otvalid/otvgdef.c \
    $$PWD/src/otvalid/otvgpos.c \
    $$PWD/src/otvalid/otvgsub.c \
    $$PWD/src/otvalid/otvjstf.c \
    $$PWD/src/otvalid/otvmod.c \
    $$PWD/src/pcf/pcf.c \
    $$PWD/src/pfr/pfr.c \
    $$PWD/src/psaux/psaux.c \
    $$PWD/src/pshinter/pshinter.c \
    $$PWD/src/psnames/psmodule.c \
    $$PWD/src/raster/raster.c \
    $$PWD/src/sfnt/sfnt.c \
    $$PWD/src/smooth/smooth.c \
    $$PWD/src/truetype/truetype.c \
    $$PWD/src/type1/type1.c \
    $$PWD/src/type42/type42.c \
    $$PWD/src/winfonts/winfnt.c

win32 {
    SOURCES += $$PWD/src/base/ftsystem.c
} else {
    SOURCES += $$PWD/builds/unix/ftsystem.c
    INCLUDEPATH += $$PWD/builds/unix
}

DEFINES += FT2_BUILD_LIBRARY

DEFINES += FT_CONFIG_OPTION_SYSTEM_ZLIB
include(../zlib_dependency.pri)

DEFINES += FT_CONFIG_OPTION_USE_PNG
include($$OUT_PWD/../../gui/qtgui-config.pri)
QMAKE_USE_PRIVATE += libpng

DEFINES += TT_CONFIG_OPTION_SUBPIXEL_HINTING

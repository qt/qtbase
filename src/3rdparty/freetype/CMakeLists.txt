# Generated from freetype.pro.

#####################################################################
## BundledFreetype Generic Library:
#####################################################################

qt_internal_add_3rdparty_library(BundledFreetype
    QMAKE_LIB_NAME freetype
    STATIC
    INSTALL
    SOURCES
        src/autofit/autofit.c
        src/base/ftbase.c
        src/base/ftbbox.c
        src/base/ftbitmap.c
        src/base/ftdebug.c
        src/base/ftglyph.c
        src/base/ftinit.c
        src/base/ftmm.c
        src/base/ftsynth.c
        src/base/fttype1.c
        src/bdf/bdf.c
        src/cache/ftcache.c
        src/cff/cff.c
        src/cid/type1cid.c
        src/gzip/ftgzip.c
        src/lzw/ftlzw.c
        src/otvalid/otvalid.c
        src/otvalid/otvbase.c
        src/pcf/pcf.c
        src/pfr/pfr.c
        src/psaux/psaux.c
        src/pshinter/pshinter.c
        src/psnames/psmodule.c
        src/raster/raster.c
        src/sfnt/sfnt.c
        src/smooth/smooth.c
        src/truetype/truetype.c
        src/type1/type1.c
        src/type42/type42.c
        src/winfonts/winfnt.c
    DEFINES
        FT2_BUILD_LIBRARY
        FT_CONFIG_OPTION_SYSTEM_ZLIB
        TT_CONFIG_OPTION_SUBPIXEL_HINTING
    PUBLIC_INCLUDE_DIRECTORIES
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
qt_disable_warnings(BundledFreetype)
qt_set_symbol_visibility_hidden(BundledFreetype)

#### Keys ignored in scope 1:.:.:freetype.pro:<TRUE>:
# MODULE_EXT_HEADERS_DIR = "$$PWD/include"
# OTHER_FILES = "$$PWD/src/autofit/afangles.c" "$$PWD/src/autofit/afglobal.c" "$$PWD/src/autofit/afloader.c" "$$PWD/src/autofit/afmodule.c"
# QT_FOR_CONFIG = "gui-private"

## Scopes:
#####################################################################

qt_internal_extend_target(BundledFreetype CONDITION WIN32
    SOURCES
        src/base/ftsystem.c
)

qt_internal_extend_target(BundledFreetype CONDITION UNIX
    SOURCES
        builds/unix/ftsystem.c
    INCLUDE_DIRECTORIES
        builds/unix
)

qt_internal_extend_target(BundledFreetype CONDITION QT_FEATURE_png
    DEFINES
        FT_CONFIG_OPTION_USE_PNG
    LIBRARIES
        WrapPNG::WrapPNG
)

qt_internal_extend_target(BundledFreetype CONDITION QT_FEATURE_system_zlib
    LIBRARIES
        WrapZLIB::WrapZLIB
)

qt_internal_extend_target(BundledFreetype CONDITION NOT QT_FEATURE_system_zlib
    INCLUDE_DIRECTORIES
        ../zlib/src
)

qt_internal_extend_target(BundledFreetype CONDITION NOT QT_FEATURE_system_zlib AND NOT no_core_dep
    LIBRARIES
        Qt::Core
)

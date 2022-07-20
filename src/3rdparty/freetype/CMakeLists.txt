# From freetype CMakeLists.txt

set(BASE_SRCS
  src/autofit/autofit.c
  src/base/ftbase.c
  src/base/ftbbox.c
  src/base/ftbdf.c
  src/base/ftbitmap.c
  src/base/ftcid.c
  src/base/ftfstype.c
  src/base/ftgasp.c
  src/base/ftglyph.c
  src/base/ftgxval.c
  src/base/ftinit.c
  src/base/ftmm.c
  src/base/ftotval.c
  src/base/ftpatent.c
  src/base/ftpfr.c
  src/base/ftstroke.c
  src/base/ftsynth.c
  src/base/fttype1.c
  src/base/ftwinfnt.c
  src/bdf/bdf.c
  src/bzip2/ftbzip2.c
  src/cache/ftcache.c
  src/cff/cff.c
  src/cid/type1cid.c
  src/gzip/ftgzip.c
  src/lzw/ftlzw.c
  src/pcf/pcf.c
  src/pfr/pfr.c
  src/psaux/psaux.c
  src/pshinter/pshinter.c
  src/psnames/psnames.c
  src/raster/raster.c
  src/sdf/sdf.c
  src/sfnt/sfnt.c
  src/smooth/smooth.c
  src/svg/svg.c
  src/truetype/truetype.c
  src/type1/type1.c
  src/type42/type42.c
  src/winfonts/winfnt.c
)

if (WIN32)
#  enable_language(RC)
  list(APPEND BASE_SRCS builds/windows/ftdebug.c)
#                        src/base/ftver.rc)
elseif (WINCE)
  list(APPEND BASE_SRCS builds/wince/ftdebug.c)
else ()
  list(APPEND BASE_SRCS src/base/ftdebug.c)
endif ()


#####################################################################
## BundledFreetype Generic Library:
#####################################################################

qt_internal_add_3rdparty_library(BundledFreetype
    QMAKE_LIB_NAME freetype
    STATIC
    SKIP_AUTOMOC
    INSTALL
    SOURCES
        ${BASE_SRCS}
    DEFINES
        FT2_BUILD_LIBRARY
        FT_CONFIG_OPTION_SYSTEM_ZLIB
        TT_CONFIG_OPTION_SUBPIXEL_HINTING
    PUBLIC_INCLUDE_DIRECTORIES
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

qt_internal_add_3rdparty_header_module(FreetypePrivate
    EXTERNAL_HEADERS_DIR include
)

qt_disable_warnings(BundledFreetype)
qt_set_symbol_visibility_hidden(BundledFreetype)

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

#####################################################################
## BundledLibpng Generic Library:
#####################################################################

qt_internal_add_3rdparty_library(BundledLibpng
    QMAKE_LIB_NAME libpng
    STATIC
    SKIP_AUTOMOC
    INSTALL
    SOURCES
        png.c
        pngerror.c
        pngget.c
        pngmem.c
        pngpread.c
        pngread.c
        pngrio.c
        pngrtran.c
        pngrutil.c
        pngset.c
        pngtrans.c
        pngwio.c
        pngwrite.c
        pngwtran.c
        pngwutil.c
    DEFINES
        PNG_ARM_NEON_OPT=0
        PNG_POWERPC_VSX_OPT=0
        PNG_IMPEXP=
        _CRT_SECURE_NO_DEPRECATE
    PUBLIC_INCLUDE_DIRECTORIES
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

qt_internal_add_3rdparty_header_module(PngPrivate
    EXTERNAL_HEADERS
        png.h
        pngconf.h
        pnglibconf.h
)

qt_disable_warnings(BundledLibpng)
qt_set_symbol_visibility_hidden(BundledLibpng)

qt_internal_extend_target(BundledLibpng CONDITION QT_FEATURE_system_zlib
    LIBRARIES
        WrapZLIB::WrapZLIB
)

qt_internal_extend_target(BundledLibpng CONDITION NOT QT_FEATURE_system_zlib
    INCLUDE_DIRECTORIES
        ../zlib/src
)

qt_internal_extend_target(BundledLibpng CONDITION NOT QT_FEATURE_system_zlib AND NOT no_core_dep
    LIBRARIES
        Qt::Core
)

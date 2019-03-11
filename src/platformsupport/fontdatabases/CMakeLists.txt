# Generated from fontdatabases.pro.

#####################################################################
## FontDatabaseSupport Module:
#####################################################################

find_package(Freetype) # Special case

add_qt_module(FontDatabaseSupport
    STATIC
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    # CONFIG = "static" "internal_module"
    # MODULE = "fontdatabase_support"
    # PRECOMPILED_HEADER = "../../corelib/global/qt_pch.h"
    # _LOADED = "qt_module"
)

## Scopes:
#####################################################################

extend_target(FontDatabaseSupport CONDITION APPLE
    SOURCES
        mac/qcoretextfontdatabase.mm mac/qcoretextfontdatabase_p.h
        mac/qfontengine_coretext.mm mac/qfontengine_coretext_p.h
    LIBRARIES
        ${FWCoreFoundation}
        ${FWCoreGraphics}
        ${FWCoreText}
        ${FWFoundation}
        ${FWAppKit} # Special case

    # CONFIG = "watchos_coretext"
)

extend_target(FontDatabaseSupport CONDITION APPLE AND QT_FEATURE_freetype
    SOURCES
        freetype/qfontengine_ft.cpp freetype/qfontengine_ft_p.h
    LIBRARIES
        Freetype::Freetype
)

extend_target(FontDatabaseSupport CONDITION QT_FEATURE_freetype
    SOURCES
        freetype/qfontengine_ft.cpp freetype/qfontengine_ft_p.h
        freetype/qfreetypefontdatabase.cpp freetype/qfreetypefontdatabase_p.h
    LIBRARIES
        Freetype::Freetype
)

extend_target(FontDatabaseSupport CONDITION UNIX
    SOURCES
        genericunix/qgenericunixfontdatabase_p.h
)

extend_target(FontDatabaseSupport CONDITION QT_FEATURE_fontconfig AND UNIX
    SOURCES
        fontconfig/qfontconfigdatabase.cpp fontconfig/qfontconfigdatabase_p.h
        fontconfig/qfontenginemultifontconfig.cpp fontconfig/qfontenginemultifontconfig_p.h
    LIBRARIES
        fontconfig
)

extend_target(FontDatabaseSupport CONDITION WIN32 AND NOT WINRT
    SOURCES
        windows/qwindowsfontdatabase.cpp windows/qwindowsfontdatabase_p.h
        windows/qwindowsfontengine.cpp windows/qwindowsfontengine_p.h
        windows/qwindowsnativeimage.cpp windows/qwindowsnativeimage_p.h
    LIBRARIES
        Qt::GuiPrivate
        ole32
        gdi32
        user32
        advapi32
)

extend_target(FontDatabaseSupport CONDITION QT_FEATURE_freetype AND WIN32 AND NOT WINRT
    SOURCES
        windows/qwindowsfontdatabase_ft.cpp windows/qwindowsfontdatabase_ft_p.h
)

extend_target(FontDatabaseSupport CONDITION QT_FEATURE_directwrite AND WIN32 AND NOT WINRT
    SOURCES
        windows/qwindowsfontenginedirectwrite.cpp windows/qwindowsfontenginedirectwrite_p.h
)

extend_target(FontDatabaseSupport CONDITION QT_FEATURE_directwrite AND QT_FEATURE_directwrite2 AND WIN32 AND NOT WINRT
    DEFINES
        QT_USE_DIRECTWRITE2
)

extend_target(FontDatabaseSupport CONDITION WIN32 AND NOT QT_FEATURE_directwrite AND NOT WINRT
    DEFINES
        QT_NO_DIRECTWRITE
)

extend_target(FontDatabaseSupport CONDITION WIN32 AND mingw AND NOT WINRT
    LIBRARIES
        uuid
)

extend_target(FontDatabaseSupport CONDITION WINRT
    SOURCES
        winrt/qwinrtfontdatabase.cpp winrt/qwinrtfontdatabase_p.h
    DEFINES
        __WRL_NO_DEFAULT_LIB__
    LIBRARIES
        Qt::GuiPrivate
        ws2_32
        dwrite
)

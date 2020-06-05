# Generated from fontdatabases.pro.

qt_find_package(WrapFreetype PROVIDED_TARGETS WrapFreetype::WrapFreetype) # special case
qt_find_package(Fontconfig) # special case

#####################################################################
## FontDatabaseSupport Module:
#####################################################################

qt_add_module(FontDatabaseSupport
    STATIC
    INTERNAL_MODULE
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PRECOMPILED_HEADER
        "../../corelib/global/qt_pch.h"
)

#### Keys ignored in scope 1:.:.:fontdatabases.pro:<TRUE>:
# MODULE = "fontdatabase_support"

## Scopes:
#####################################################################

qt_extend_target(FontDatabaseSupport CONDITION UNIX
    SOURCES
        genericunix/qgenericunixfontdatabase_p.h
)

qt_extend_target(FontDatabaseSupport CONDITION QT_FEATURE_fontconfig
    SOURCES
        fontconfig/qfontconfigdatabase.cpp fontconfig/qfontconfigdatabase_p.h
        fontconfig/qfontenginemultifontconfig.cpp fontconfig/qfontenginemultifontconfig_p.h
    LIBRARIES
        Fontconfig::Fontconfig
)

# special case begin
if(TARGET WrapFreetype::WrapFreetype)
   qt_extend_target(FontDatabaseSupport CONDITION QT_FEATURE_fontconfig
       LIBRARIES
           WrapFreetype::WrapFreetype
)
endif()
# special case end

qt_extend_target(FontDatabaseSupport CONDITION WIN32
    SOURCES
        windows/qwindowsfontdatabase.cpp windows/qwindowsfontdatabase_p.h
        windows/qwindowsfontdatabasebase.cpp windows/qwindowsfontdatabasebase_p.h
        windows/qwindowsfontengine.cpp windows/qwindowsfontengine_p.h
        windows/qwindowsnativeimage.cpp windows/qwindowsnativeimage_p.h
    LIBRARIES
        advapi32
        gdi32
        ole32
        user32
    PUBLIC_LIBRARIES
        Qt::GuiPrivate
)

qt_extend_target(FontDatabaseSupport CONDITION QT_FEATURE_freetype AND WIN32
    SOURCES
        windows/qwindowsfontdatabase_ft.cpp windows/qwindowsfontdatabase_ft_p.h
    LIBRARIES
        WrapFreetype::WrapFreetype
)

qt_extend_target(FontDatabaseSupport CONDITION QT_FEATURE_direct2d AND QT_FEATURE_directwrite AND WIN32
    SOURCES
        windows/qwindowsfontenginedirectwrite.cpp windows/qwindowsfontenginedirectwrite_p.h
    LIBRARIES
        d2d1
)

qt_extend_target(FontDatabaseSupport CONDITION QT_FEATURE_direct2d AND QT_FEATURE_directwrite AND QT_FEATURE_directwrite3 AND WIN32
    SOURCES
        windows/qwindowsdirectwritefontdatabase.cpp windows/qwindowsdirectwritefontdatabase_p.h
    DEFINES
        QT_USE_DIRECTWRITE2
        QT_USE_DIRECTWRITE3
    LIBRARIES
        dwrite_3
)

qt_extend_target(FontDatabaseSupport CONDITION QT_FEATURE_direct2d AND QT_FEATURE_directwrite AND QT_FEATURE_directwrite2 AND WIN32 AND NOT QT_FEATURE_directwrite3
    DEFINES
        QT_USE_DIRECTWRITE2
    LIBRARIES
        dwrite_2
)

qt_extend_target(FontDatabaseSupport CONDITION QT_FEATURE_direct2d AND QT_FEATURE_directwrite AND WIN32 AND NOT QT_FEATURE_directwrite2 AND NOT QT_FEATURE_directwrite3
    LIBRARIES
        dwrite
)

qt_extend_target(FontDatabaseSupport CONDITION WIN32 AND (NOT QT_FEATURE_direct2d OR NOT QT_FEATURE_directwrite)
    DEFINES
        QT_NO_DIRECTWRITE
)

qt_extend_target(FontDatabaseSupport CONDITION MINGW AND WIN32
    LIBRARIES
        uuid
)

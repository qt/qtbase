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

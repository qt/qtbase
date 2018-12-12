# Generated from fontdatabases.pro.

#####################################################################
## QtFontDatabaseSupport Module:
#####################################################################

find_package(Freetype)
find_library(FWCoreGraphics CoreGraphics)
find_library(FWCoreFoundation CoreFoundation)
find_library(FWCoreText CoreText)
find_library(FWFoundation Foundation)
find_library(FWAppKit AppKit)

add_qt_module(FontDatabaseSupport
    STATIC
    SOURCES fake.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
)
set_source_files_properties(fake.cpp PROPERTIES
    SKIP_AUTOMOC ON
    SKIP_AUTOUIC ON
    SKIP_AUTORCC ON
)

extend_target(FontDatabaseSupport CONDITION APPLE
    SOURCES
        mac/qcoretextfontdatabase.mm mac/qcoretextfontdatabase_p.h
        mac/qfontengine_coretext.mm mac/qfontengine_coretext_p.h
    LIBRARIES
        ${FWCoreFoundation}
        ${FWCoreGraphics}
        ${FWCoreText}
        ${FWFoundation}
        ${FWAppKit}
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

extend_target(FontDatabaseSupport CONDITION UNIX AND QT_FEATURE_fontconfig
    SOURCES
        fontconfig/qfontconfigdatabase.cpp fontconfig/qfontconfigdatabase_p.h
        fontconfig/qfontenginemultifontconfig.cpp fontconfig/qfontenginemultifontconfig_p.h
    LIBRARIES
        fontconfig
)

#extend_target(FontDatabaseSupport CONDITION WIN32
#)

#extend_target(FontDatabaseSupport CONDITION (WIN32) AND (NOT WINRT)
#    SOURCES
#        qwindowsfontdatabase.cpp qwindowsfontdatabase_p.h
#        qwindowsfontengine.cpp qwindowsfontengine_p.h
#        qwindowsnativeimage.cpp qwindowsnativeimage_p.h
#    LIBRARIES
#        Qt::GuiPrivate
#)

#extend_target(FontDatabaseSupport CONDITION ((WIN32) AND (NOT WINRT)) AND (QT_FEATURE_freetype)
#    SOURCES
#        qwindowsfontdatabase_ft.cpp qwindowsfontdatabase_ft_p.h
#)

#extend_target(FontDatabaseSupport CONDITION ((WIN32) AND (NOT WINRT)) AND (QT_FEATURE_directwrite)
#    SOURCES
#        qwindowsfontenginedirectwrite.cpp qwindowsfontenginedirectwrite_p.h
#)

#extend_target(FontDatabaseSupport CONDITION (((WIN32) AND (NOT WINRT)) AND (QT_FEATURE_directwrite)) AND (QT_FEATURE_directwrite2)
#    DEFINES
#        QT_USE_DIRECTWRITE2
#)

#extend_target(FontDatabaseSupport CONDITION ((WIN32) AND (NOT WINRT)) AND (NOT QT_FEATURE_directwrite)
#    DEFINES
#        QT_NO_DIRECTWRITE
#)
##
##extend_target(FontDatabaseSupport CONDITION ((WIN32) AND (NOT WINRT)) AND (mingw)
##)

#extend_target(FontDatabaseSupport CONDITION WINRT
#    SOURCES
#        qwinrtfontdatabase.cpp qwinrtfontdatabase_p.h
#    DEFINES
#        __WRL_NO_DEFAULT_LIB__
#    LIBRARIES
#        Qt::GuiPrivate
#)

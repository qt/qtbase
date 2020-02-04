# -*-mode:sh-*-
# Qt image handling

# Qt kernel module

HEADERS += \
        image/qbitmap.h \
        image/qimage.h \
        image/qimage_p.h \
        image/qimageiohandler.h \
        image/qimagereader.h \
        image/qimagereaderwriterhelpers_p.h \
        image/qimagewriter.h \
        image/qpaintengine_pic_p.h \
        image/qpicture.h \
        image/qpicture_p.h \
        image/qpictureformatplugin.h \
        image/qpixmap.h \
        image/qpixmap_raster_p.h \
        image/qpixmap_blitter_p.h \
        image/qpixmapcache.h \
        image/qpixmapcache_p.h \
        image/qplatformpixmap.h \
        image/qimagepixmapcleanuphooks_p.h \
        image/qicon.h \
        image/qicon_p.h \
        image/qiconloader_p.h \
        image/qiconengine.h \
        image/qiconengineplugin.h \

SOURCES += \
        image/qbitmap.cpp \
        image/qimage.cpp \
        image/qimage_conversions.cpp \
        image/qimageiohandler.cpp \
        image/qimagereader.cpp \
        image/qimagereaderwriterhelpers.cpp \
        image/qimagewriter.cpp \
        image/qpaintengine_pic.cpp \
        image/qpicture.cpp \
        image/qpictureformatplugin.cpp \
        image/qpixmap.cpp \
        image/qpixmapcache.cpp \
        image/qplatformpixmap.cpp \
        image/qpixmap_raster.cpp \
        image/qpixmap_blitter.cpp \
        image/qimagepixmapcleanuphooks.cpp \
        image/qicon.cpp \
        image/qiconloader.cpp \
        image/qiconengine.cpp \
        image/qiconengineplugin.cpp \

qtConfig(movie) {
    HEADERS += image/qmovie.h
    SOURCES += image/qmovie.cpp
}

win32:!winrt: SOURCES += image/qpixmap_win.cpp

darwin: OBJECTIVE_SOURCES += image/qimage_darwin.mm

NO_PCH_SOURCES += image/qimage_compat.cpp
false: SOURCES += $$NO_PCH_SOURCES # Hack for QtCreator

# Built-in image format support
HEADERS += \
        image/qbmphandler_p.h \
        image/qppmhandler_p.h \
        image/qxbmhandler_p.h \
        image/qxpmhandler_p.h

SOURCES += \
        image/qbmphandler.cpp \
        image/qppmhandler.cpp \
        image/qxbmhandler.cpp \
        image/qxpmhandler.cpp

qtConfig(png) {
    HEADERS += image/qpnghandler_p.h
    SOURCES += image/qpnghandler.cpp
    QMAKE_USE_PRIVATE += libpng

    win32:mingw {
      # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86048
      GCC_VERSION = "$${QMAKE_GCC_MAJOR_VERSION}.$${QMAKE_GCC_MINOR_VERSION}.$${QMAKE_GCC_PATCH_VERSION}"
      equals(GCC_VERSION, "8.1.0") {
        QMAKE_CXXFLAGS += -fno-reorder-blocks-and-partition
      }
    }
}

# SIMD
!android {
    SSSE3_SOURCES += image/qimage_ssse3.cpp
    NEON_SOURCES += image/qimage_neon.cpp
    MIPS_DSPR2_SOURCES += image/qimage_mips_dspr2.cpp
    MIPS_DSPR2_ASM += image/qimage_mips_dspr2_asm.S
} else {
    # see https://developer.android.com/ndk/guides/abis
    arm64-v8a | armeabi-v7a {
        SOURCES += image/qimage_neon.cpp
    }
    x86 | x86_64 {
        DEFINES += QT_COMPILER_SUPPORTS_SSE2 QT_COMPILER_SUPPORTS_SSE3 QT_COMPILER_SUPPORTS_SSSE3
        SOURCES += image/qimage_ssse3.cpp
    }
}

# Generated from jpeg.pro.

qt_find_package(JPEG) # special case

#####################################################################
## QJpegPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QJpegPlugin
    OUTPUT_NAME qjpeg
    PLUGIN_TYPE imageformats
    SOURCES
        main.cpp
        qjpeghandler.cpp qjpeghandler_p.h
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

# Fails to build on Windows with a static Qt, PCH enabled and
# the vendored libjpeg sources, due to 'boolean'
# redefinition in jmorecfg.h and rpcndr.h.
if(WIN32 AND NOT BUILD_SHARED_LIBS)
    qt_update_ignore_pch_source(QJpegPlugin "qjpeghandler.cpp")
endif()

#### Keys ignored in scope 1:.:.:jpeg.pro:<TRUE>:
# OTHER_FILES = "jpeg.json"

## Scopes:
#####################################################################

qt_internal_extend_target(QJpegPlugin CONDITION QT_FEATURE_system_jpeg
    LIBRARIES
        JPEG::JPEG
)

qt_internal_extend_target(QJpegPlugin CONDITION NOT QT_FEATURE_system_jpeg
    SOURCES
        ../../../3rdparty/libjpeg/src/jaricom.c
        ../../../3rdparty/libjpeg/src/jcapimin.c
        ../../../3rdparty/libjpeg/src/jcapistd.c
        ../../../3rdparty/libjpeg/src/jcarith.c
        ../../../3rdparty/libjpeg/src/jccoefct.c
        ../../../3rdparty/libjpeg/src/jccolor.c
        ../../../3rdparty/libjpeg/src/jcdctmgr.c
        ../../../3rdparty/libjpeg/src/jchuff.c
        ../../../3rdparty/libjpeg/src/jcinit.c
        ../../../3rdparty/libjpeg/src/jcmainct.c
        ../../../3rdparty/libjpeg/src/jcmarker.c
        ../../../3rdparty/libjpeg/src/jcmaster.c
        ../../../3rdparty/libjpeg/src/jcomapi.c
        ../../../3rdparty/libjpeg/src/jcparam.c
        ../../../3rdparty/libjpeg/src/jcphuff.c
        ../../../3rdparty/libjpeg/src/jcprepct.c
        ../../../3rdparty/libjpeg/src/jcsample.c
        ../../../3rdparty/libjpeg/src/jctrans.c
        ../../../3rdparty/libjpeg/src/jdapimin.c
        ../../../3rdparty/libjpeg/src/jdapistd.c
        ../../../3rdparty/libjpeg/src/jdarith.c
        ../../../3rdparty/libjpeg/src/jdatadst.c
        ../../../3rdparty/libjpeg/src/jdatasrc.c
        ../../../3rdparty/libjpeg/src/jdcoefct.c
        ../../../3rdparty/libjpeg/src/jdcolor.c
        ../../../3rdparty/libjpeg/src/jddctmgr.c
        ../../../3rdparty/libjpeg/src/jdhuff.c
        ../../../3rdparty/libjpeg/src/jdinput.c
        ../../../3rdparty/libjpeg/src/jdmainct.c
        ../../../3rdparty/libjpeg/src/jdmarker.c
        ../../../3rdparty/libjpeg/src/jdmaster.c
        ../../../3rdparty/libjpeg/src/jdmerge.c
        ../../../3rdparty/libjpeg/src/jdphuff.c
        ../../../3rdparty/libjpeg/src/jdpostct.c
        ../../../3rdparty/libjpeg/src/jdsample.c
        ../../../3rdparty/libjpeg/src/jdtrans.c
        ../../../3rdparty/libjpeg/src/jerror.c
        ../../../3rdparty/libjpeg/src/jfdctflt.c
        ../../../3rdparty/libjpeg/src/jfdctfst.c
        ../../../3rdparty/libjpeg/src/jfdctint.c
        ../../../3rdparty/libjpeg/src/jidctflt.c
        ../../../3rdparty/libjpeg/src/jidctfst.c
        ../../../3rdparty/libjpeg/src/jidctint.c
        ../../../3rdparty/libjpeg/src/jidctred.c
        ../../../3rdparty/libjpeg/src/jmemmgr.c
        ../../../3rdparty/libjpeg/src/jmemnobs.c
        ../../../3rdparty/libjpeg/src/jquant1.c
        ../../../3rdparty/libjpeg/src/jquant2.c
        ../../../3rdparty/libjpeg/src/jsimd_none.c
        ../../../3rdparty/libjpeg/src/jutils.c
    INCLUDE_DIRECTORIES
        ../../../3rdparty/libjpeg/src
)

# special case begin
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"
    OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
    OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    target_compile_options(QJpegPlugin PRIVATE "-Wno-unused-parameter")
endif()
# special case end

#### Keys ignored in scope 5:.:../../../3rdparty:../../../3rdparty/libjpeg.pri:GCC:
# QMAKE_CFLAGS_WARN_ON = "-Wno-unused-parameter" "-Wno-main"

qt_internal_extend_target(QJpegPlugin CONDITION MSVC AND NOT QT_FEATURE_system_jpeg
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)

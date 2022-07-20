qt_internal_add_3rdparty_library(BundledLibjpeg
    QMAKE_LIB_NAME libjpeg
    STATIC
    SKIP_AUTOMOC
    INSTALL
    SOURCES
        src/jaricom.c
        src/jcapimin.c
        src/jcapistd.c
        src/jcarith.c
        src/jccoefct.c
        src/jccolor.c
        src/jcdctmgr.c
        src/jchuff.c
        src/jcinit.c
        src/jcmainct.c
        src/jcmarker.c
        src/jcmaster.c
        src/jcomapi.c
        src/jcparam.c
        src/jcphuff.c
        src/jcprepct.c
        src/jcsample.c
        src/jctrans.c
        src/jdapimin.c
        src/jdapistd.c
        src/jdarith.c
        src/jdatadst.c
        src/jdatasrc.c
        src/jdcoefct.c
        src/jdcolor.c
        src/jddctmgr.c
        src/jdhuff.c
        src/jdinput.c
        src/jdmainct.c
        src/jdmarker.c
        src/jdmaster.c
        src/jdmerge.c
        src/jdphuff.c
        src/jdpostct.c
        src/jdsample.c
        src/jdtrans.c
        src/jerror.c
        src/jfdctflt.c
        src/jfdctfst.c
        src/jfdctint.c
        src/jidctflt.c
        src/jidctfst.c
        src/jidctint.c
        src/jidctred.c
        src/jmemmgr.c
        src/jmemnobs.c
        src/jquant1.c
        src/jquant2.c
        src/jsimd_none.c
        src/jutils.c
    INCLUDE_DIRECTORIES
        src
    PUBLIC_INCLUDE_DIRECTORIES
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

qt_internal_add_3rdparty_header_module(JpegPrivate
    EXTERNAL_HEADERS
        src/jpeglib.h
        src/jerror.h
        src/jconfig.h
        src/jmorecfg.h
)

qt_disable_warnings(BundledLibjpeg)
qt_set_symbol_visibility_hidden(BundledLibjpeg)

qt_internal_extend_target(BundledLibjpeg CONDITION MSVC
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"
    OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
    OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    target_compile_options(BundledLibjpeg PRIVATE "-Wno-unused-parameter")
endif()


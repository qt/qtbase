set(JPEG16_SOURCES
    src/jcapistd.c
    src/jccolor.c
    src/jcdiffct.c
    src/jclossls.c
    src/jcmainct.c
    src/jcprepct.c
    src/jcsample.c
    src/jdapistd.c
    src/jdcolor.c
    src/jddiffct.c
    src/jdlossls.c
    src/jdmainct.c
    src/jdpostct.c
    src/jdsample.c
    src/jutils.c)

set(JPEG12_SOURCES
    ${JPEG16_SOURCES}
    src/jccoefct.c
    src/jcdctmgr.c
    src/jdcoefct.c
    src/jddctmgr.c
    src/jdmerge.c
    src/jfdctfst.c
    src/jfdctint.c
    src/jidctflt.c
    src/jidctfst.c
    src/jidctint.c
    src/jidctred.c
    src/jquant1.c
    src/jquant2.c)

set(JPEG_SOURCES
    ${JPEG12_SOURCES}
    src/jaricom.c
    src/jcapimin.c
    src/jcarith.c
    src/jchuff.c
    src/jcicc.c
    src/jcinit.c
    src/jclhuff.c
    src/jcmarker.c
    src/jcmaster.c
    src/jcomapi.c
    src/jcparam.c
    src/jcphuff.c
    src/jctrans.c
    src/jdapimin.c
    src/jdarith.c
    src/jdatadst.c
    src/jdatasrc.c
    src/jdhuff.c
    src/jdicc.c
    src/jdinput.c
    src/jdlhuff.c
    src/jdmarker.c
    src/jdmaster.c
    src/jdphuff.c
    src/jdtrans.c
    src/jerror.c
    src/jfdctflt.c
    src/jmemmgr.c
    src/jmemnobs.c)

qt_internal_add_3rdparty_library(BundledLibjpeg16bits
    STATIC
    SKIP_AUTOMOC
    SOURCES
        ${JPEG16_SOURCES}
    DEFINES
        BITS_IN_JSAMPLE=16
    INCLUDE_DIRECTORIES
        src
    PUBLIC_INCLUDE_DIRECTORIES
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

qt_internal_add_3rdparty_library(BundledLibjpeg12bits
    STATIC
    SKIP_AUTOMOC
    SOURCES
        ${JPEG12_SOURCES}
    DEFINES
        BITS_IN_JSAMPLE=12
    INCLUDE_DIRECTORIES
        src
    PUBLIC_INCLUDE_DIRECTORIES
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

qt_internal_add_3rdparty_library(BundledLibjpeg
    QMAKE_LIB_NAME libjpeg
    STATIC
    SKIP_AUTOMOC
    INSTALL
    SOURCES
        ${JPEG_SOURCES}
        $<TARGET_OBJECTS:BundledLibjpeg12bits>
        $<TARGET_OBJECTS:BundledLibjpeg16bits>
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
qt_disable_warnings(BundledLibjpeg12bits)
qt_disable_warnings(BundledLibjpeg16bits)
qt_set_symbol_visibility_hidden(BundledLibjpeg)
qt_set_symbol_visibility_hidden(BundledLibjpeg12bits)
qt_set_symbol_visibility_hidden(BundledLibjpeg16bits)

qt_internal_extend_target(BundledLibjpeg CONDITION MSVC
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"
    OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
    OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    target_compile_options(BundledLibjpeg PRIVATE "-Wno-unused-parameter")
endif()


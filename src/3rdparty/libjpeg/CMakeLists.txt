# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
set(jpeg16_original_sources
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

set(jpeg12_original_sources
    ${jpeg16_original_sources}
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

set(jpeg_original_sources
    ${jpeg12_original_sources}
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
    src/jmemnobs.c
    src/jpeg_nbits.c)

# Make copies of the source files for the different bit counts, so that all the object file names
# are unique in the static archive. This avoids duplicate warnings for certain linkers and debug
# info extractors (like Apple's ld and dsymutil).
function(qt_internal_copy_jpeg_sources in_sources infix out_var)
    set(jpeg_sources "")
    foreach(src_file ${in_sources})
        get_filename_component(src_file_name "${src_file}" NAME_WLE)
        get_filename_component(src_file_extension "${src_file}" LAST_EXT)
        set(dest_path "${CMAKE_CURRENT_BINARY_DIR}/${src_file_name}${infix}${src_file_extension}")
        configure_file(${src_file} "${dest_path}" COPYONLY)
        message(DEBUG "Copying jpeg source file ${src_file} to ${dest_path}.")
        list(APPEND jpeg_sources "${dest_path}")
    endforeach()
    set(${out_var} "${jpeg_sources}" PARENT_SCOPE)
endfunction()

qt_internal_copy_jpeg_sources("${jpeg16_original_sources}" "_16" jpeg16_sources)
qt_internal_copy_jpeg_sources("${jpeg12_original_sources}" "_12" jpeg12_sources)
qt_internal_copy_jpeg_sources("${jpeg_original_sources}" "" jpeg_sources)

qt_internal_add_3rdparty_library(BundledLibjpeg16bits
    STATIC
    SKIP_AUTOMOC
    SOURCES
        ${jpeg16_sources}
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
        ${jpeg12_sources}
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
        ${jpeg_sources}
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

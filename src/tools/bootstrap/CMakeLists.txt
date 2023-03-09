# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## Bootstrap Module:
#####################################################################

# The bootstrap library has a few manual tweaks compared to other
# libraries.
qt_add_library(Bootstrap STATIC)

qt_internal_add_sync_header_dependencies(Bootstrap Core)

qt_internal_extend_target(Bootstrap
    SOURCES
        ../../corelib/global/qassert.cpp
        ../../corelib/global/qendian.cpp
        ../../corelib/global/qfloat16.cpp
        ../../corelib/global/qglobal.cpp
        ../../corelib/global/qlogging.cpp
        ../../corelib/global/qmalloc.cpp
        ../../corelib/global/qnumeric.cpp
        ../../corelib/global/qoperatingsystemversion.cpp
        ../../corelib/global/qrandom.cpp
        ../../corelib/global/qtenvironmentvariables.cpp
        ../../corelib/io/qabstractfileengine.cpp
        ../../corelib/io/qbuffer.cpp
        ../../corelib/io/qdebug.cpp
        ../../corelib/io/qdir.cpp
        ../../corelib/io/qdiriterator.cpp
        ../../corelib/io/qfile.cpp
        ../../corelib/io/qfiledevice.cpp
        ../../corelib/io/qfileinfo.cpp
        ../../corelib/io/qfilesystemengine.cpp
        ../../corelib/io/qfilesystementry.cpp
        ../../corelib/io/qfsfileengine.cpp
        ../../corelib/io/qfsfileengine_iterator.cpp
        ../../corelib/io/qiodevice.cpp
        ../../corelib/io/qloggingcategory.cpp
        ../../corelib/io/qloggingregistry.cpp
        ../../corelib/io/qresource.cpp
        ../../corelib/io/qsavefile.cpp
        ../../corelib/io/qstandardpaths.cpp
        ../../corelib/io/qtemporaryfile.cpp
        ../../corelib/kernel/qcoreapplication.cpp
        ../../corelib/kernel/qiterable.cpp
        ../../corelib/kernel/qmetacontainer.cpp
        ../../corelib/kernel/qmetatype.cpp
        ../../corelib/kernel/qsystemerror.cpp
        ../../corelib/kernel/qvariant.cpp
        ../../corelib/plugin/quuid.cpp
        ../../corelib/serialization/qcborcommon.cpp
        ../../corelib/serialization/qcborstreamwriter.cpp
        ../../corelib/serialization/qcborvalue.cpp
        ../../corelib/serialization/qdatastream.cpp
        ../../corelib/serialization/qjsonarray.cpp
        ../../corelib/serialization/qjsoncbor.cpp
        ../../corelib/serialization/qjsondocument.cpp
        ../../corelib/serialization/qjsonobject.cpp
        ../../corelib/serialization/qjsonparser.cpp
        ../../corelib/serialization/qjsonvalue.cpp
        ../../corelib/serialization/qjsonwriter.cpp
        ../../corelib/serialization/qtextstream.cpp
        ../../corelib/text/qbytearray.cpp
        ../../corelib/text/qbytearraylist.cpp
        ../../corelib/text/qbytearraymatcher.cpp
        ../../corelib/text/qlatin1stringmatcher.cpp
        ../../corelib/text/qlocale.cpp
        ../../corelib/text/qlocale_tools.cpp
        ../../corelib/text/qregularexpression.cpp
        ../../corelib/text/qstring.cpp
        ../../corelib/text/qstringbuilder.cpp
        ../../corelib/text/qstringconverter.cpp
        ../../corelib/text/qstringlist.cpp
        ../../corelib/text/qvsnprintf.cpp
        ../../corelib/time/qcalendar.cpp
        ../../corelib/time/qdatetime.cpp
        ../../corelib/time/qgregoriancalendar.cpp
        ../../corelib/time/qlocaltime.cpp
        ../../corelib/time/qromancalendar.cpp
        ../../corelib/time/qtimezone.cpp
        ../../corelib/tools/qarraydata.cpp
        ../../corelib/tools/qbitarray.cpp
        ../../corelib/tools/qcommandlineoption.cpp
        ../../corelib/tools/qcommandlineparser.cpp
        ../../corelib/tools/qcryptographichash.cpp
        ../../corelib/tools/qhash.cpp
        ../../corelib/tools/qringbuffer.cpp
        ../../corelib/tools/qversionnumber.cpp
    DEFINES
        HAVE_CONFIG_H
        QT_TYPESAFE_FLAGS
    PUBLIC_DEFINES
        QT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        QT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        QT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        QT_VERSION_STR="${PROJECT_VERSION}"
        QT_USE_QSTRINGBUILDER
        QT_BOOTSTRAPPED
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        ..
        ../../3rdparty/tinycbor/src
    PUBLIC_INCLUDE_DIRECTORIES
        $<TARGET_PROPERTY:Core,INCLUDE_DIRECTORIES>
        ../../corelib/global
    PUBLIC_LIBRARIES
        Qt::Platform
    NO_UNITY_BUILD
)

## Scopes:
#####################################################################

qt_internal_extend_target(Bootstrap CONDITION UNIX
    SOURCES
        ../../corelib/io/qfilesystemengine_unix.cpp
        ../../corelib/io/qfilesystemiterator_unix.cpp
        ../../corelib/io/qfsfileengine_unix.cpp
        ../../corelib/kernel/qcore_unix.cpp
)

qt_internal_extend_target(Bootstrap CONDITION WIN32
    SOURCES
        ../../corelib/global/qoperatingsystemversion_win.cpp
        ../../corelib/io/qfilesystemengine_win.cpp
        ../../corelib/io/qfilesystemiterator_win.cpp
        ../../corelib/io/qfsfileengine_win.cpp
        ../../corelib/io/qstandardpaths_win.cpp
        ../../corelib/kernel/qcoreapplication_win.cpp
        ../../corelib/kernel/qwinregistry.cpp
        ../../corelib/plugin/qsystemlibrary.cpp
        ../../corelib/kernel/qfunctions_win.cpp
    PUBLIC_LIBRARIES
        advapi32
        netapi32
        ole32
        shell32
        user32
)

qt_internal_extend_target(Bootstrap CONDITION APPLE
    SOURCES
        ../../corelib/global/qoperatingsystemversion_darwin.mm
        ../../corelib/kernel/qcore_foundation.mm
        ../../corelib/kernel/qcore_mac.mm
        ../../corelib/kernel/qcoreapplication_mac.cpp
    PUBLIC_LIBRARIES
        ${FWFoundation}
)

qt_internal_extend_target(Bootstrap CONDITION MACOS
    SOURCES
        ../../corelib/io/qstandardpaths_mac.mm
    LIBRARIES
        ${FWCoreServices}
)

qt_internal_extend_target(Bootstrap CONDITION UIKIT
    LIBRARIES
        ${FWUIKit}
)

qt_internal_extend_target(Bootstrap CONDITION UNIX AND NOT MACOS
    SOURCES
        ../../corelib/io/qstandardpaths_unix.cpp
)

qt_internal_extend_target(Bootstrap CONDITION CMAKE_CROSSCOMPILING OR NOT QT_FEATURE_system_pcre2
    SOURCES
        ../../3rdparty/pcre2/src/config.h
        ../../3rdparty/pcre2/src/pcre2.h
        ../../3rdparty/pcre2/src/pcre2_auto_possess.c
        ../../3rdparty/pcre2/src/pcre2_chartables.c
        ../../3rdparty/pcre2/src/pcre2_compile.c
        ../../3rdparty/pcre2/src/pcre2_config.c
        ../../3rdparty/pcre2/src/pcre2_context.c
        ../../3rdparty/pcre2/src/pcre2_dfa_match.c
        ../../3rdparty/pcre2/src/pcre2_error.c
        ../../3rdparty/pcre2/src/pcre2_extuni.c
        ../../3rdparty/pcre2/src/pcre2_find_bracket.c
        ../../3rdparty/pcre2/src/pcre2_internal.h
        ../../3rdparty/pcre2/src/pcre2_intmodedep.h
        ../../3rdparty/pcre2/src/pcre2_jit_compile.c
        ../../3rdparty/pcre2/src/pcre2_maketables.c
        ../../3rdparty/pcre2/src/pcre2_match.c
        ../../3rdparty/pcre2/src/pcre2_match_data.c
        ../../3rdparty/pcre2/src/pcre2_newline.c
        ../../3rdparty/pcre2/src/pcre2_ord2utf.c
        ../../3rdparty/pcre2/src/pcre2_pattern_info.c
        ../../3rdparty/pcre2/src/pcre2_script_run.c
        ../../3rdparty/pcre2/src/pcre2_serialize.c
        ../../3rdparty/pcre2/src/pcre2_string_utils.c
        ../../3rdparty/pcre2/src/pcre2_study.c
        ../../3rdparty/pcre2/src/pcre2_substitute.c
        ../../3rdparty/pcre2/src/pcre2_substring.c
        ../../3rdparty/pcre2/src/pcre2_tables.c
        ../../3rdparty/pcre2/src/pcre2_ucd.c
        ../../3rdparty/pcre2/src/pcre2_ucp.h
        ../../3rdparty/pcre2/src/pcre2_valid_utf.c
        ../../3rdparty/pcre2/src/pcre2_xclass.c
    INCLUDE_DIRECTORIES
        ../../3rdparty/pcre2/src
    DEFINES
        PCRE2_CODE_UNIT_WIDTH=16
        PCRE2_DISABLE_JIT
    PUBLIC_INCLUDE_DIRECTORIES
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/pcre2/src>
)

qt_internal_extend_target(Bootstrap
    CONDITION WIN32 AND (CMAKE_CROSSCOMPILING OR NOT QT_FEATURE_system_pcre2)
    PUBLIC_DEFINES PCRE2_STATIC
)

qt_internal_extend_target(Bootstrap CONDITION QT_FEATURE_system_pcre2 AND NOT CMAKE_CROSSCOMPILING
    LIBRARIES
        WrapPCRE2::WrapPCRE2
)

qt_internal_extend_target(Bootstrap CONDITION MINGW AND WIN32
    PUBLIC_LIBRARIES
        uuid
)

target_link_libraries(Bootstrap PRIVATE PlatformCommonInternal)
qt_internal_apply_gc_binaries(Bootstrap PUBLIC)
set_target_properties(Bootstrap PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
qt_internal_add_target_aliases(Bootstrap)
qt_set_msvc_cplusplus_options(Bootstrap PUBLIC)
qt_set_common_target_properties(Bootstrap)
qt_internal_apply_intel_cet(Bootstrap PUBLIC)

if(WARNINGS_ARE_ERRORS)
    qt_internal_set_warnings_are_errors_flags(Bootstrap PRIVATE)
endif()
qt_internal_extend_target(Bootstrap CONDITION MSVC
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.20.0" AND QT_FEATURE_debug_and_release)
    set_property(TARGET Bootstrap
        PROPERTY EXCLUDE_FROM_ALL "$<NOT:$<CONFIG:${QT_MULTI_CONFIG_FIRST_CONFIG}>>")
endif()

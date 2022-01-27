# Generated from bootstrap.pro.

#####################################################################
## Bootstrap Module:
#####################################################################

# special case begin
# The bootstrap library has a few manual tweaks compared to other
# libraries.
qt_add_library(Bootstrap STATIC)
# special case end
qt_internal_extend_target(Bootstrap
    SOURCES
        ../../corelib/global/qendian.cpp
        ../../corelib/global/qfloat16.cpp
        ../../corelib/global/qglobal.cpp
        ../../corelib/global/qlogging.cpp
        ../../corelib/global/qmalloc.cpp
        ../../corelib/global/qnumeric.cpp
        ../../corelib/global/qoperatingsystemversion.cpp
        ../../corelib/global/qrandom.cpp
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
        ../../corelib/io/qipaddress.cpp
        ../../corelib/io/qloggingcategory.cpp
        ../../corelib/io/qloggingregistry.cpp
        ../../corelib/io/qresource.cpp
        ../../corelib/io/qsavefile.cpp
        ../../corelib/io/qstandardpaths.cpp
        ../../corelib/io/qtemporarydir.cpp
        ../../corelib/io/qtemporaryfile.cpp
        ../../corelib/io/qurl.cpp
        ../../corelib/io/qurlidna.cpp
        ../../corelib/io/qurlquery.cpp
        ../../corelib/io/qurlrecode.cpp
        ../../corelib/kernel/qcoreapplication.cpp
        ../../corelib/kernel/qcoreglobaldata.cpp
        ../../corelib/kernel/qiterable.cpp
        ../../corelib/kernel/qmetacontainer.cpp
        ../../corelib/kernel/qmetatype.cpp
        ../../corelib/kernel/qsystemerror.cpp
        ../../corelib/kernel/qsystemsemaphore.cpp
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
        ../../corelib/serialization/qxmlstream.cpp
        ../../corelib/serialization/qxmlstreamgrammar.cpp
        ../../corelib/serialization/qxmlutils.cpp
        ../../corelib/text/qbytearray.cpp
        ../../corelib/text/qbytearraylist.cpp
        ../../corelib/text/qbytearraymatcher.cpp
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
        ../../corelib/time/qromancalendar.cpp
        ../../corelib/tools/qarraydata.cpp
        ../../corelib/tools/qbitarray.cpp
        ../../corelib/tools/qcommandlineoption.cpp
        ../../corelib/tools/qcommandlineparser.cpp
        ../../corelib/tools/qcryptographichash.cpp
        ../../corelib/tools/qhash.cpp
        ../../corelib/tools/qline.cpp
        ../../corelib/tools/qpoint.cpp
        ../../corelib/tools/qrect.cpp
        ../../corelib/tools/qringbuffer.cpp
        ../../corelib/tools/qsize.cpp
        ../../corelib/tools/qversionnumber.cpp
        ../../xml/dom/qdom.cpp
    DEFINES
        HAVE_CONFIG_H
    PUBLIC_DEFINES # special case
        QT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR} # special case
        QT_VERSION_MINOR=${PROJECT_VERSION_MINOR} # special case
        QT_VERSION_PATCH=${PROJECT_VERSION_PATCH} # special case
        QT_VERSION_STR="${PROJECT_VERSION}" # special case
        QT_USE_QSTRINGBUILDER # special case
        QT_BOOTSTRAPPED
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        ..
        ../../3rdparty/tinycbor/src
    PUBLIC_INCLUDE_DIRECTORIES  # special case
        $<TARGET_PROPERTY:Core,INCLUDE_DIRECTORIES> # special case
        $<TARGET_PROPERTY:Xml,INCLUDE_DIRECTORIES> # special case
    PUBLIC_LIBRARIES  # special case
        Qt::Platform # special case
)

#### Keys ignored in scope 1:.:.:bootstrap.pro:<TRUE>:
# INSTALLS = "lib"
# MODULE_CONFIG = "gc_binaries"
# MODULE_INCNAME = "QtCore" "QtXml"
# _OPTION = "host_build"
# lib.CONFIG = "dummy_install"

## Scopes:
#####################################################################

qt_internal_extend_target(Bootstrap CONDITION UNIX
    SOURCES
        ../../corelib/io/qfilesystemengine_unix.cpp
        ../../corelib/io/qfilesystemiterator_unix.cpp
        ../../corelib/io/qfsfileengine_unix.cpp
        ../../corelib/kernel/qcore_unix.cpp
        ../../corelib/kernel/qsystemsemaphore_posix.cpp
        ../../corelib/kernel/qsystemsemaphore_systemv.cpp
        ../../corelib/kernel/qsystemsemaphore_unix.cpp
)

qt_internal_extend_target(Bootstrap CONDITION WIN32
    SOURCES
        ../../corelib/global/qoperatingsystemversion_win.cpp
        ../../corelib/io/qfilesystemengine_win.cpp
        ../../corelib/io/qfilesystemiterator_win.cpp
        ../../corelib/io/qfsfileengine_win.cpp
        ../../corelib/io/qstandardpaths_win.cpp
        ../../corelib/kernel/qcoreapplication_win.cpp
        ../../corelib/kernel/qsystemsemaphore_win.cpp
        ../../corelib/kernel/qwinregistry.cpp
        ../../corelib/plugin/qsystemlibrary.cpp
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
    PUBLIC_INCLUDE_DIRECTORIES  # special case
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

qt_internal_extend_target(Bootstrap CONDITION CMAKE_CROSSCOMPILING OR NOT QT_FEATURE_system_zlib
    SOURCES
        ../../3rdparty/zlib/src/adler32.c
        ../../3rdparty/zlib/src/compress.c
        ../../3rdparty/zlib/src/crc32.c
        ../../3rdparty/zlib/src/deflate.c
        ../../3rdparty/zlib/src/gzclose.c
        ../../3rdparty/zlib/src/gzlib.c
        ../../3rdparty/zlib/src/gzread.c
        ../../3rdparty/zlib/src/gzwrite.c
        ../../3rdparty/zlib/src/infback.c
        ../../3rdparty/zlib/src/inffast.c
        ../../3rdparty/zlib/src/inflate.c
        ../../3rdparty/zlib/src/inftrees.c
        ../../3rdparty/zlib/src/trees.c
        ../../3rdparty/zlib/src/uncompr.c
        ../../3rdparty/zlib/src/zutil.c
    INCLUDE_DIRECTORIES
        ../../3rdparty/zlib/src
)

qt_internal_extend_target(Bootstrap CONDITION QT_FEATURE_system_zlib AND NOT CMAKE_CROSSCOMPILING
    LIBRARIES
        WrapZLIB::WrapZLIB
)

qt_internal_extend_target(Bootstrap CONDITION MINGW AND WIN32
    PUBLIC_LIBRARIES
        uuid
)

#### Keys ignored in scope 22:.:../../3rdparty/pcre2:../../3rdparty/pcre2/pcre2.pri:QT_FEATURE_intelcet:
# QMAKE_CFLAGS = "$$QMAKE_CFLAGS_SHSTK"

# special case begin
target_link_libraries(Bootstrap PRIVATE PlatformCommonInternal)
qt_internal_apply_gc_binaries(Bootstrap PUBLIC)
set_target_properties(Bootstrap PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
qt_internal_add_target_aliases(Bootstrap)
qt_set_msvc_cplusplus_options(Bootstrap PUBLIC)
qt_set_common_target_properties(Bootstrap)
qt_internal_apply_intel_cet(Bootstrap PUBLIC)

qt_internal_extend_target(Bootstrap CONDITION MSVC
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.20.0" AND QT_FEATURE_debug_and_release)
    set_property(TARGET Bootstrap
        PROPERTY EXCLUDE_FROM_ALL "$<NOT:$<CONFIG:${QT_MULTI_CONFIG_FIRST_CONFIG}>>")
endif()
# special case end

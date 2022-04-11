# Generated from bootstrap.pro.

#####################################################################
## Bootstrap Module:
#####################################################################

# special case begin
# The bootstrap library has a few manual tweaks compared to other
# libraries.
qt_add_library(Bootstrap STATIC)

option(QT_USE_BOOTSTRAP_SOURCE_COPY "Use copies of source files for building the bootstrap lib."
    ${QT_FEATURE_private_tests})

if(QT_USE_BOOTSTRAP_SOURCE_COPY)
    # To avoid unnecessary rebuilds whenever a source file of the bootstrap library is touched, we
    # use a copy of the corelib sources. This copy can be updated manually by building the target
    # 'update_bootstrap_sources'.

    set(source_root ${CMAKE_CURRENT_BINARY_DIR}/srccopy)
    if(NOT IS_DIRECTORY "${source_root}")
        file(COPY "${QtBase_SOURCE_DIR}/src/corelib" DESTINATION "${source_root}")
    endif()

    file(GLOB_RECURSE source_files
        LIST_DIRECTORIES FALSE
        RELATIVE "${QtBase_SOURCE_DIR}/src/corelib"
        "${QtBase_SOURCE_DIR}/src/corelib/*")

    set(copy_commands)
    set(allowed_extensions .c .cpp .h .mm)
    foreach(f IN LISTS source_files)
        get_filename_component(ext "${f}" LAST_EXT)
        if(NOT ext IN_LIST allowed_extensions)
            continue()
        endif()
        list(APPEND copy_commands
            COMMAND "${CMAKE_COMMAND}" -E
            copy_if_different "${QtBase_SOURCE_DIR}/src/corelib/${f}"
                              "${source_root}/corelib/${f}")
    endforeach()
    set(info_msg "Please run 'cmake --build .' twice, to ensure all relevant files are rebuilt due")
    string(APPEND info_msg " to the bootstrap source file copy.")
    add_custom_target(update_bootstrap_sources
        ${copy_commands}
        COMMAND ${CMAKE_COMMAND} -E echo "${info_msg}"
        VERBATIM)
else()
    # Directly use the files in the source tree.
    set(source_root ../..)
endif()

# special case end
qt_internal_extend_target(Bootstrap
    SOURCES
        ${source_root}/corelib/global/qendian.cpp
        ${source_root}/corelib/global/qfloat16.cpp
        ${source_root}/corelib/global/qglobal.cpp
        ${source_root}/corelib/global/qlogging.cpp
        ${source_root}/corelib/global/qmalloc.cpp
        ${source_root}/corelib/global/qnumeric.cpp
        ${source_root}/corelib/global/qoperatingsystemversion.cpp
        ${source_root}/corelib/global/qrandom.cpp
        ${source_root}/corelib/io/qabstractfileengine.cpp
        ${source_root}/corelib/io/qbuffer.cpp
        ${source_root}/corelib/io/qdebug.cpp
        ${source_root}/corelib/io/qdir.cpp
        ${source_root}/corelib/io/qdiriterator.cpp
        ${source_root}/corelib/io/qfile.cpp
        ${source_root}/corelib/io/qfiledevice.cpp
        ${source_root}/corelib/io/qfileinfo.cpp
        ${source_root}/corelib/io/qfilesystemengine.cpp
        ${source_root}/corelib/io/qfilesystementry.cpp
        ${source_root}/corelib/io/qfsfileengine.cpp
        ${source_root}/corelib/io/qfsfileengine_iterator.cpp
        ${source_root}/corelib/io/qiodevice.cpp
        ${source_root}/corelib/io/qloggingcategory.cpp
        ${source_root}/corelib/io/qloggingregistry.cpp
        ${source_root}/corelib/io/qresource.cpp
        ${source_root}/corelib/io/qsavefile.cpp
        ${source_root}/corelib/io/qstandardpaths.cpp
        ${source_root}/corelib/kernel/qcoreapplication.cpp
        ${source_root}/corelib/kernel/qcoreglobaldata.cpp
        ${source_root}/corelib/kernel/qiterable.cpp
        ${source_root}/corelib/kernel/qmetacontainer.cpp
        ${source_root}/corelib/kernel/qmetatype.cpp
        ${source_root}/corelib/kernel/qsystemerror.cpp
        ${source_root}/corelib/kernel/qvariant.cpp
        ${source_root}/corelib/plugin/quuid.cpp
        ${source_root}/corelib/serialization/qcborcommon.cpp
        ${source_root}/corelib/serialization/qcborstreamwriter.cpp
        ${source_root}/corelib/serialization/qcborvalue.cpp
        ${source_root}/corelib/serialization/qdatastream.cpp
        ${source_root}/corelib/serialization/qjsonarray.cpp
        ${source_root}/corelib/serialization/qjsoncbor.cpp
        ${source_root}/corelib/serialization/qjsondocument.cpp
        ${source_root}/corelib/serialization/qjsonobject.cpp
        ${source_root}/corelib/serialization/qjsonparser.cpp
        ${source_root}/corelib/serialization/qjsonvalue.cpp
        ${source_root}/corelib/serialization/qjsonwriter.cpp
        ${source_root}/corelib/serialization/qtextstream.cpp
        ${source_root}/corelib/text/qbytearray.cpp
        ${source_root}/corelib/text/qbytearraylist.cpp
        ${source_root}/corelib/text/qbytearraymatcher.cpp
        ${source_root}/corelib/text/qlocale.cpp
        ${source_root}/corelib/text/qlocale_tools.cpp
        ${source_root}/corelib/text/qregularexpression.cpp
        ${source_root}/corelib/text/qstring.cpp
        ${source_root}/corelib/text/qstringbuilder.cpp
        ${source_root}/corelib/text/qstringconverter.cpp
        ${source_root}/corelib/text/qstringlist.cpp
        ${source_root}/corelib/text/qvsnprintf.cpp
        ${source_root}/corelib/time/qcalendar.cpp
        ${source_root}/corelib/time/qdatetime.cpp
        ${source_root}/corelib/time/qgregoriancalendar.cpp
        ${source_root}/corelib/time/qromancalendar.cpp
        ${source_root}/corelib/tools/qarraydata.cpp
        ${source_root}/corelib/tools/qbitarray.cpp
        ${source_root}/corelib/tools/qcommandlineoption.cpp
        ${source_root}/corelib/tools/qcommandlineparser.cpp
        ${source_root}/corelib/tools/qcryptographichash.cpp
        ${source_root}/corelib/tools/qhash.cpp
        ${source_root}/corelib/tools/qringbuffer.cpp
        ${source_root}/corelib/tools/qversionnumber.cpp
    DEFINES
        HAVE_CONFIG_H
        QT_TYPESAFE_FLAGS
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
# special case end

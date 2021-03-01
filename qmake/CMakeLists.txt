# Generated from qmake.pro.

#####################################################################
## qmake Tool:
#####################################################################

# TODO: Probably it's time to introduce QT_FEATURE_qmake?
if(NOT (QT_FEATURE_settings AND QT_FEATURE_alloca AND (QT_FEATURE_alloca_malloc_h OR NOT WIN32) AND
        QT_FEATURE_cborstreamwriter AND QT_FEATURE_datestring AND QT_FEATURE_regularexpression AND
        QT_FEATURE_temporaryfile))
    message(WARNING "Skip building qmake in specified configuration.\
Required features:
   QT_FEATURE_settings ${QT_FEATURE_settings}
   QT_FEATURE_alloca ${QT_FEATURE_alloca}
   QT_FEATURE_cborstreamwriter ${QT_FEATURE_cborstreamwriter}
   QT_FEATURE_datestring ${QT_FEATURE_datestring}
   QT_FEATURE_getauxval ${QT_FEATURE_getauxval}
   QT_FEATURE_regularexpression ${QT_FEATURE_regularexpression}
   QT_FEATURE_temporaryfile ${QT_FEATURE_temporaryfile}
")
    return()
endif()

# The QT_CONFIGURE_HOSTBINDIR_TO_*PREFIX_PATH defines are exclusively used by qmake to determine
# the prefix from the location of the qmake executable. In our build of qmake host_prefix is
# always the same as ext_prefix, and we can just use CMAKE_INSTALL_PREFIX for the calculation of
# the relative path between <ext_prefix>/bin and <ext_prefix>.
file(RELATIVE_PATH QT_CONFIGURE_RELATIVE_PREFIX_PATH
    "${CMAKE_INSTALL_PREFIX}/${INSTALL_BINDIR}" "${CMAKE_INSTALL_PREFIX}"
)

qt_get_tool_target_name(target_name qmake)
qt_internal_add_tool(${target_name}
    TOOLS_TARGET Core # special case
    SOURCES
        cachekeys.h
        generators/mac/pbuilder_pbx.cpp generators/mac/pbuilder_pbx.h
        generators/makefile.cpp generators/makefile.h
        generators/makefiledeps.cpp generators/makefiledeps.h
        generators/metamakefile.cpp generators/metamakefile.h
        generators/projectgenerator.cpp generators/projectgenerator.h
        generators/unix/unixmake.cpp generators/unix/unixmake.h
        generators/unix/unixmake2.cpp
        generators/win32/mingw_make.cpp generators/win32/mingw_make.h
        generators/win32/msbuild_objectmodel.cpp generators/win32/msbuild_objectmodel.h
        generators/win32/msvc_nmake.cpp generators/win32/msvc_nmake.h
        generators/win32/msvc_objectmodel.cpp generators/win32/msvc_objectmodel.h
        generators/win32/msvc_vcproj.cpp generators/win32/msvc_vcproj.h
        generators/win32/msvc_vcxproj.cpp generators/win32/msvc_vcxproj.h
        generators/win32/winmakefile.cpp generators/win32/winmakefile.h
        generators/xmloutput.cpp generators/xmloutput.h
        library/ioutils.cpp library/ioutils.h
        library/proitems.cpp library/proitems.h
        library/qmake_global.h
        library/qmakebuiltins.cpp
        library/qmakeevaluator.cpp library/qmakeevaluator.h library/qmakeevaluator_p.h
        library/qmakeglobals.cpp library/qmakeglobals.h
        library/qmakeparser.cpp library/qmakeparser.h
        library/qmakevfs.cpp library/qmakevfs.h
        qmakelibraryinfo.cpp qmakelibraryinfo.h
        main.cpp
        meta.cpp meta.h
        option.cpp option.h
        project.cpp project.h
        property.cpp property.h
    DEFINES
        HAVE_CONFIG_H
        PROEVALUATOR_FULL
        QT_BUILD_QMAKE
        QT_USE_QSTRINGBUILDER
        QT_NO_FOREACH
        QT_VERSION_STR="${PROJECT_VERSION}" # special case
        QT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR} # special case
        QT_VERSION_MINOR=${PROJECT_VERSION_MINOR} # special case
        QT_VERSION_PATCH=${PROJECT_VERSION_PATCH} # special case
        QT_CONFIGURE_RELATIVE_PREFIX_PATH="${QT_CONFIGURE_RELATIVE_PREFIX_PATH}"
    INCLUDE_DIRECTORIES
        generators
        generators/mac
        generators/unix
        generators/win32
        library
        "${CMAKE_CURRENT_BINARY_DIR}/library" # for qmakeconfig.cpp
    PRECOMPILED_HEADER
        "qmake_pch.h"
    LIBRARIES
        Qt::CorePrivate
)

qt_generate_qconfig_cpp(../src/corelib/global/qconfig.cpp.in library/qmakeconfig.cpp)

#### Keys ignored in scope 1:.:.:qmake.pro:<TRUE>:
# _OPTION = "host_build"

## Scopes:
#####################################################################

qt_internal_extend_target(${target_name} CONDITION MACOS
    COMPILE_OPTIONS
        "-fconstant-cfstrings"
)

# special case big
qt_internal_extend_target(${target_name} CONDITION WIN32
    SOURCES
        library/registry.cpp
    DEFINES
        _SCL_SECURE_NO_WARNINGS
)

qt_internal_extend_target(${target_name} CONDITION CLANG AND WIN32
    COMPILE_OPTIONS
        "-fms-compatibility-version=19.00.23506"
        "-Wno-microsoft-enum-value"
)

# special case:
set_target_properties(${target_name} PROPERTIES
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
)

qt_internal_apply_gc_binaries(${target_name} PRIVATE) # special case
qt_skip_warnings_are_errors(${target_name}) # special case

# special case begin
qt_internal_add_docs(${target_name}
    doc/qmake.qdocconf
)
# special case end


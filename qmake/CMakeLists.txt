# Generated from qmake.pro.

#####################################################################
## qmake Tool:
#####################################################################

# The common object library, that should be available regardless of the presence of qmake.
qt_add_library(QtLibraryInfo OBJECT
    library/proitems.cpp library/proitems.h
    library/qmake_global.h
    property.cpp property.h
    propertyprinter.cpp propertyprinter.h
    qmakelibraryinfo.cpp qmakelibraryinfo.h
)
set_target_properties(QtLibraryInfo PROPERTIES
    COMPILE_OPTIONS $<TARGET_PROPERTY:Qt::Core,INTERFACE_COMPILE_OPTIONS>
    COMPILE_DEFINITIONS $<TARGET_PROPERTY:Qt::Core,INTERFACE_COMPILE_DEFINITIONS>
    INCLUDE_DIRECTORIES $<TARGET_PROPERTY:Qt::Core,INTERFACE_INCLUDE_DIRECTORIES>
    INCLUDE_DIRECTORIES $<TARGET_PROPERTY:Qt::CorePrivate,INTERFACE_INCLUDE_DIRECTORIES>
)
target_include_directories(QtLibraryInfo PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/library"
)

# Chop off the "/mkspecs" part of INSTALL_MKSPECSDIR
get_filename_component(hostdatadir "${INSTALL_MKSPECSDIR}" DIRECTORY)
if("${hostdatadir}" STREQUAL "")
    set(hostdatadir ".")
endif()

target_compile_definitions(QtLibraryInfo PUBLIC
    PROEVALUATOR_FULL
    QT_BUILD_QMAKE
    QT_USE_QSTRINGBUILDER
    QT_NO_FOREACH
    QT_HOST_MKSPEC="${QT_QMAKE_HOST_MKSPEC}"
    QT_TARGET_MKSPEC="${QT_QMAKE_TARGET_MKSPEC}"
    QT_HOST_DATADIR="${hostdatadir}"
)

if(NOT QT_FEATURE_qmake)
    return()
endif()

qt_get_tool_target_name(target_name qmake)
qt_internal_add_tool(${target_name}
    TOOLS_TARGET Core # special case
    USER_FACING
    INSTALL_VERSIONED_LINK
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
        library/qmakebuiltins.cpp
        library/qmakeevaluator.cpp library/qmakeevaluator.h library/qmakeevaluator_p.h
        library/qmakeglobals.cpp library/qmakeglobals.h
        library/qmakeparser.cpp library/qmakeparser.h
        library/qmakevfs.cpp library/qmakevfs.h
        main.cpp
        meta.cpp meta.h
        option.cpp option.h
        project.cpp project.h
    DEFINES
        HAVE_CONFIG_H
    INCLUDE_DIRECTORIES
        generators
        generators/mac
        generators/unix
        generators/win32
        library
    PRECOMPILED_HEADER
        "qmake_pch.h"
    LIBRARIES
        Qt::CorePrivate
        QtLibraryInfo
)
qt_internal_return_unless_building_tools()

# Add QMAKE_VERSION_STR only if qmake is part of the build.
target_compile_definitions(QtLibraryInfo PUBLIC
        QMAKE_VERSION_STR="3.1"
)

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

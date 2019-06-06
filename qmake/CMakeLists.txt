# Generated from qmake.pro.

# special case begin
set(path_component "${INSTALL_DATADIR}")
qt_path_join(mkspecs_install_dir ${QT_INSTALL_DIR} ${path_component})

qt_copy_or_install(DIRECTORY "${PROJECT_SOURCE_DIR}/mkspecs"
                   DESTINATION ${mkspecs_install_dir})
# special case end

#####################################################################
## qmake Binary:
#####################################################################

add_qt_tool(qmake # special case
    NO_QT # special case
    TOOLS_TARGET Core # special case
    # GUI # special case: remove this
    SOURCES
        ../src/corelib/codecs/qutfcodec.cpp ../src/corelib/codecs/qutfcodec_p.h
        ../src/corelib/global/qendian.cpp # special case
        ../src/corelib/global/qglobal.cpp ../src/corelib/global/qglobal.h
        ../src/corelib/global/qlibraryinfo.cpp
        ../src/corelib/global/qlogging.cpp
        ../src/corelib/global/qmalloc.cpp
        ../src/corelib/global/qnumeric.cpp ../src/corelib/global/qnumeric.h
        ../src/corelib/global/qoperatingsystemversion.cpp # special case
        ../src/corelib/global/qrandom.cpp # special case
        ../src/corelib/io/qabstractfileengine.cpp ../src/corelib/io/qabstractfileengine_p.h
        ../src/corelib/io/qbuffer.cpp ../src/corelib/io/qbuffer.h
        ../src/corelib/io/qdebug.cpp # special case
        ../src/corelib/io/qdir.cpp ../src/corelib/io/qdir.h ../src/corelib/io/qdir_p.h
        ../src/corelib/io/qdiriterator.cpp ../src/corelib/io/qdiriterator.h
        ../src/corelib/io/qfile.cpp ../src/corelib/io/qfile.h
        ../src/corelib/io/qfiledevice.cpp
        ../src/corelib/io/qfileinfo.cpp ../src/corelib/io/qfileinfo.h
        ../src/corelib/io/qfilesystemengine.cpp
        ../src/corelib/io/qfilesystementry.cpp
        ../src/corelib/io/qfsfileengine.cpp
        ../src/corelib/io/qfsfileengine_iterator.cpp
        ../src/corelib/io/qiodevice.cpp ../src/corelib/io/qiodevice.h
        ../src/corelib/io/qsettings.cpp
        ../src/corelib/io/qtemporaryfile.cpp ../src/corelib/io/qtemporaryfile.h
        ../src/corelib/kernel/qmetatype.cpp ../src/corelib/kernel/qmetatype.h
        ../src/corelib/kernel/qsystemerror.cpp ../src/corelib/kernel/qsystemerror_p.h
        ../src/corelib/kernel/qvariant.cpp
        ../src/corelib/plugin/quuid.cpp ../src/corelib/plugin/quuid.h
        ../src/corelib/serialization/qjson.cpp ../src/corelib/serialization/qjson_p.h
        ../src/corelib/serialization/qjsonarray.cpp ../src/corelib/serialization/qjsonarray.h
        ../src/corelib/serialization/qjsondocument.cpp ../src/corelib/serialization/qjsondocument.h
        ../src/corelib/serialization/qjsonobject.cpp ../src/corelib/serialization/qjsonobject.h
        ../src/corelib/serialization/qjsonparser.cpp ../src/corelib/serialization/qjsonparser_p.h
        ../src/corelib/serialization/qjsonvalue.cpp ../src/corelib/serialization/qjsonvalue.h
        ../src/corelib/serialization/qjsonwriter_p.h
        ../src/corelib/serialization/qtextstream.cpp ../src/corelib/serialization/qtextstream.h
        ../src/corelib/serialization/qxmlstream.cpp ../src/corelib/serialization/qxmlstream.h
        ../src/corelib/serialization/qxmlutils.cpp ../src/corelib/serialization/qxmlutils_p.h
        ../src/corelib/tools/qarraydata.cpp ../src/corelib/tools/qarraydata.h
        ../src/corelib/tools/qarraydataops.h
        ../src/corelib/tools/qarraydatapointer.h
        ../src/corelib/tools/qbitarray.cpp ../src/corelib/tools/qbitarray.h
        ../src/corelib/tools/qbytearray.cpp ../src/corelib/tools/qbytearray.h
        ../src/corelib/tools/qbytearraymatcher.cpp ../src/corelib/tools/qbytearraymatcher.h
        ../src/corelib/tools/qchar.h
        ../src/corelib/tools/qcryptographichash.cpp ../src/corelib/tools/qcryptographichash.h
        ../src/corelib/tools/qdatetime.cpp ../src/corelib/tools/qdatetime.h ../src/corelib/tools/qdatetime_p.h
        ../src/corelib/tools/qhash.cpp ../src/corelib/tools/qhash.h
        ../src/corelib/tools/qlist.cpp ../src/corelib/tools/qlist.h
        ../src/corelib/tools/qlocale.cpp ../src/corelib/tools/qlocale.h
        ../src/corelib/tools/qlocale_tools.cpp ../src/corelib/tools/qlocale_tools_p.h
        ../src/corelib/tools/qmap.cpp ../src/corelib/tools/qmap.h
        ../src/corelib/tools/qregexp.cpp ../src/corelib/tools/qregexp.h
        ../src/corelib/tools/qringbuffer.cpp # special case
        ../src/corelib/tools/qstring.cpp ../src/corelib/tools/qstring.h
        ../src/corelib/tools/qstringlist.cpp ../src/corelib/tools/qstringlist.h
        ../src/corelib/tools/qstringmatcher.h
        ../src/corelib/tools/qvector.h
        ../src/corelib/tools/qversionnumber.cpp ../src/corelib/tools/qversionnumber.h
        ../src/corelib/tools/qvsnprintf.cpp
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
        main.cpp
        meta.cpp meta.h
        option.cpp option.h
        project.cpp project.h
        property.cpp property.h
    DEFINES
        PROEVALUATOR_FULL
        QT_BOOTSTRAPPED
        QT_BUILD_QMAKE
        QT_NO_FOREACH
        QT_VERSION_STR="${PROJECT_VERSION}" # special case
        QT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR} # special case
        QT_VERSION_MINOR=${PROJECT_VERSION_MINOR} # special case
        QT_VERSION_PATCH=${PROJECT_VERSION_PATCH} # special case
    INCLUDE_DIRECTORIES
        # . # special case
        library
        generators
        generators/mac
        generators/unix
        generators/win32
        library
        $<TARGET_PROPERTY:Qt::CorePrivate,INTERFACE_INCLUDE_DIRECTORIES> # special case
        ${CMAKE_CURRENT_BINARY_DIR}/../src/corelib/global # special case: for qconfig.cpp
)

qt_internal_add_target_aliases(Bootstrap) # special case

#### Keys ignored in scope 1:.:.:qmake.pro:<TRUE>:
# CONFIG = "cmdline" "-qt"
# PRECOMPILED_HEADER = "qmake_pch.h"
# _OPTION = "host_build"

## Scopes:
#####################################################################

extend_target(qmake CONDITION WIN32
    SOURCES
        ../src/corelib/global/qoperatingsystemversion_win.cpp
        ../src/corelib/io/qfilesystemengine_win.cpp
        ../src/corelib/io/qfilesystemiterator_win.cpp
        ../src/corelib/io/qfsfileengine_win.cpp
        ../src/corelib/io/qsettings_win.cpp
        ../src/corelib/plugin/qsystemlibrary.cpp
        ../src/corelib/tools/qlocale_win.cpp
        library/registry.cpp # special case
    DEFINES
        UNICODE
        _CRT_SECURE_NO_WARNINGS
        _ENABLE_EXTENDED_ALIGNED_STORAGE
        _SCL_SECURE_NO_WARNINGS
    PUBLIC_LIBRARIES
        advapi32
        kernel32
        netapi32
        ole32
)

extend_target(qmake CONDITION UNIX
    SOURCES
        ../src/corelib/io/qfilesystemengine_unix.cpp
        ../src/corelib/io/qfilesystemiterator_unix.cpp
        ../src/corelib/io/qfsfileengine_unix.cpp
        ../src/corelib/kernel/qcore_unix.cpp
        ../src/corelib/tools/qlocale_unix.cpp
)

extend_target(qmake CONDITION APPLE_OSX
    SOURCES
        ../src/corelib/kernel/qcore_foundation.mm # special case
        ../src/corelib/kernel/qcore_mac.cpp # special case
        ../src/corelib/global/qoperatingsystemversion_darwin.mm # special case
        ../src/corelib/io/qsettings_mac.cpp # special case
    PUBLIC_LIBRARIES
        ${FWApplicationServices}
        ${FWCoreServices}
        ${FWFoundation}
    COMPILE_OPTIONS
        "-fconstant-cfstrings"
)

# special case big
extend_target(qmake CONDITION WIN32
    SOURCES
        ../src/corelib/io/qfilesystemengine_win.cpp
        ../src/corelib/io/qfilesystemiterator_win.cpp
        ../src/corelib/io/qfsfileengine_win.cpp
        ../src/corelib/tools/qlocale_win.cpp
        ../src/corelib/io/qsettings_win.cpp
        # ../src/corelib/global/qoperatingsystemversion.cpp special case: remove this
        ../src/corelib/plugin/qsystemlibrary.cpp
        library/registry.cpp
    DEFINES
        UNICODE _ENABLE_EXTENDED_ALIGNED_STORAGE _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS
    LIBRARIES
        ole32 advapi32 kernel32 netapi32
)

extend_target(qmake CONDITION WIN32 AND mingw
    PUBLIC_LIBRARIES
        uuid
)

extend_target(qmake CONDITION CLANG AND WIN32
    COMPILE_OPTIONS
        "-fms-compatibility-version=19.00.23506"
        "-Wno-microsoft-enum-value"
)

# special case:
set_target_properties(qmake PROPERTIES
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
)

qt_internal_add_link_flags_gc_sections(qmake PRIVATE) # special case

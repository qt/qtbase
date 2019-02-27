install(DIRECTORY "${PROJECT_SOURCE_DIR}/mkspecs" DESTINATION ${INSTALL_DATADIR})

find_library(FWApplicationServices ApplicationServices)
find_library(FWCoreServices CoreServices)
find_library(FWFoundation Foundation)

add_qt_tool(qmake
    SOURCES
        meta.cpp
        option.cpp
        project.cpp
        property.cpp
        library/ioutils.cpp
        library/proitems.cpp
        library/qmakebuiltins.cpp
        library/qmakeevaluator.cpp
        library/qmakeglobals.cpp
        library/qmakeparser.cpp
        library/qmakevfs.cpp
        generators/mac/pbuilder_pbx.cpp
        generators/makefile.cpp
        generators/makefiledeps.cpp
        generators/metamakefile.cpp
        generators/projectgenerator.cpp
        generators/unix/unixmake2.cpp
        generators/unix/unixmake.cpp
        generators/win32/mingw_make.cpp
        generators/win32/msbuild_objectmodel.cpp
        generators/win32/msvc_nmake.cpp
        generators/win32/msvc_objectmodel.cpp
        generators/win32/msvc_vcproj.cpp
        generators/win32/msvc_vcxproj.cpp
        generators/win32/winmakefile.cpp
        generators/xmloutput.cpp

        ${PROJECT_SOURCE_DIR}/src/corelib/codecs/qtextcodec.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/codecs/qutfcodec.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/global/qendian.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/global/qglobal.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/global/qlibraryinfo.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/global/qlogging.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/global/qmalloc.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/global/qnumeric.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/global/qoperatingsystemversion.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/global/qrandom.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qabstractfileengine.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qbuffer.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qdebug.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qdir.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qdiriterator.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qfile.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qfiledevice.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qfileinfo.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qfilesystemengine.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qfilesystementry.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qfsfileengine.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qfsfileengine_iterator.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qiodevice.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qsettings.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qtemporaryfile.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/kernel/qmetatype.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/kernel/qsystemerror.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/kernel/qvariant.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/plugin/quuid.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/serialization/qdatastream.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/serialization/qjsonarray.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/serialization/qjson.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/serialization/qjsondocument.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/serialization/qjsonobject.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/serialization/qjsonparser.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/serialization/qjsonvalue.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/serialization/qtextstream.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/serialization/qxmlstream.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/serialization/qxmlutils.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qarraydata.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qbitarray.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qbytearray.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qbytearraymatcher.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qcryptographichash.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qdatetime.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qhash.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qlinkedlist.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qlist.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qlocale.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qlocale_tools.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qmap.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qregexp.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qringbuffer.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qstringbuilder.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qstring_compat.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qstring.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qstringlist.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qversionnumber.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qvsnprintf.cpp

        main.cpp
    DEFINES
        QT_VERSION_STR="${PROJECT_VERSION}"
        QT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        QT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        QT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        QT_BUILD_QMAKE QT_BOOTSTRAPPED PROEVALUATOR_FULL
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        library
        generators
        generators/unix
        generators/win32
        generators/mac

        ${PROJECT_BINARY_DIR}/include
        ${PROJECT_BINARY_DIR}/include/QtCore
        ${PROJECT_BINARY_DIR}/include/QtCore/${PROJECT_VERSION}
        ${PROJECT_BINARY_DIR}/include/QtCore/${PROJECT_VERSION}/QtCore
        ${PROJECT_BINARY_DIR}/src/corelib/global
)

extend_target(qmake CONDITION UNIX
    SOURCES
        ${PROJECT_SOURCE_DIR}/src/corelib/kernel/qcore_unix.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qfilesystemengine_unix.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qfilesystemiterator_unix.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qfsfileengine_unix.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/tools/qlocale_unix.cpp
)

extend_target(qmake CONDITION APPLE_OSX
    SOURCES
        ${PROJECT_SOURCE_DIR}/src/corelib/kernel/qcore_foundation.mm
        ${PROJECT_SOURCE_DIR}/src/corelib/kernel/qcore_mac.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/global/qoperatingsystemversion_darwin.mm
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qsettings_mac.cpp
    LIBRARIES
        ${FWApplicationServices} ${FWCoreServices} ${FWFoundation}
)
# TODO: Windows support, cf. qmake.pro

set_target_properties(qmake PROPERTIES
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
)

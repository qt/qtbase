# Generated from bootstrap.pro.

#####################################################################
## Bootstrap Module:
#####################################################################

# special case:
add_library(Bootstrap STATIC)
extend_target(Bootstrap
    SOURCES
        ../../corelib/codecs/qlatincodec.cpp
        ../../corelib/codecs/qtextcodec.cpp
        ../../corelib/codecs/qutfcodec.cpp
        ../../corelib/global/qendian.cpp
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
        ../../corelib/io/qloggingcategory.cpp
        ../../corelib/io/qloggingregistry.cpp
        ../../corelib/io/qresource.cpp
        ../../corelib/io/qsavefile.cpp
        ../../corelib/io/qstandardpaths.cpp
        ../../corelib/io/qtemporarydir.cpp
        ../../corelib/io/qtemporaryfile.cpp
        ../../corelib/kernel/qcoreapplication.cpp
        ../../corelib/kernel/qcoreglobaldata.cpp
        ../../corelib/kernel/qmetatype.cpp
        ../../corelib/kernel/qsystemerror.cpp
        ../../corelib/kernel/qvariant.cpp
        ../../corelib/plugin/quuid.cpp
        ../../corelib/serialization/qdatastream.cpp
        ../../corelib/serialization/qjson.cpp
        ../../corelib/serialization/qjsonarray.cpp
        ../../corelib/serialization/qjsondocument.cpp
        ../../corelib/serialization/qjsonobject.cpp
        ../../corelib/serialization/qjsonparser.cpp
        ../../corelib/serialization/qjsonvalue.cpp
        ../../corelib/serialization/qjsonwriter.cpp
        ../../corelib/serialization/qtextstream.cpp
        ../../corelib/serialization/qxmlstream.cpp
        ../../corelib/serialization/qxmlutils.cpp
        ../../corelib/tools/qarraydata.cpp
        ../../corelib/tools/qbitarray.cpp
        ../../corelib/tools/qbytearray.cpp
        ../../corelib/tools/qbytearraymatcher.cpp
        ../../corelib/tools/qcommandlineoption.cpp
        ../../corelib/tools/qcommandlineparser.cpp
        ../../corelib/tools/qcryptographichash.cpp
        ../../corelib/tools/qdatetime.cpp
        ../../corelib/tools/qhash.cpp
        ../../corelib/tools/qline.cpp
        ../../corelib/tools/qlinkedlist.cpp
        ../../corelib/tools/qlist.cpp
        ../../corelib/tools/qlocale.cpp
        ../../corelib/tools/qlocale_tools.cpp
        ../../corelib/tools/qmap.cpp
        ../../corelib/tools/qpoint.cpp
        ../../corelib/tools/qrect.cpp
        ../../corelib/tools/qregexp.cpp
        ../../corelib/tools/qringbuffer.cpp
        ../../corelib/tools/qsize.cpp
        ../../corelib/tools/qstring.cpp
        ../../corelib/tools/qstring_compat.cpp
        ../../corelib/tools/qstringbuilder.cpp
        ../../corelib/tools/qstringlist.cpp
        ../../corelib/tools/qversionnumber.cpp
        ../../corelib/tools/qvsnprintf.cpp
        ../../xml/dom/qdom.cpp
        ../../xml/sax/qxml.cpp
    PUBLIC_DEFINES  # special case:
        QT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        QT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        QT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        QT_VERSION_STR="${PROJECT_VERSION}"
        QT_BOOTSTRAPPED
        QT_NO_CAST_ASCII
        QT_NO_FOREACH
        QT_NO_CAST_FROM_ASCII
    PUBLIC_INCLUDE_DIRECTORIES  # special case
        $<TARGET_PROPERTY:CorePrivate,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:XmlPrivate,INTERFACE_INCLUDE_DIRECTORIES>
    PUBLIC_LIBRARIES  # special case
        ZLIB::ZLIB
    # CONFIG = "minimal_syncqt" "internal_module" "force_bootstrap" "gc_binaries"
    # INSTALLS = "lib"
    # MODULE_CONFIG = "gc_binaries"
    # MODULE_DEFINES = "QT_VERSION_STR=$$shell_quote\"$$QT_VERSION\"" "QT_VERSION_MAJOR=$$QT_MAJOR_VERSION" "QT_VERSION_MINOR=$$QT_MINOR_VERSION" "QT_VERSION_PATCH=$$QT_PATCH_VERSION" "QT_BOOTSTRAPPED" "QT_NO_CAST_TO_ASCII"
    # MODULE_INCNAME = "QtCore" "QtXml"
    # _LOADED = "qt_module"
    # _OPTION = "host_build"
    # lib.CONFIG = "dummy_install"
)

qt_internal_add_target_aliases(Bootstrap)  # special case

## Scopes:
#####################################################################

extend_target(Bootstrap CONDITION UNIX
    SOURCES
        ../../corelib/io/qfilesystemengine_unix.cpp
        ../../corelib/io/qfilesystemiterator_unix.cpp
        ../../corelib/io/qfsfileengine_unix.cpp
        ../../corelib/kernel/qcore_unix.cpp
)

extend_target(Bootstrap CONDITION WIN32
    SOURCES
        ../../corelib/global/qoperatingsystemversion_win.cpp
        ../../corelib/io/qfilesystemengine_win.cpp
        ../../corelib/io/qfilesystemiterator_win.cpp
        ../../corelib/io/qfsfileengine_win.cpp
        ../../corelib/io/qstandardpaths_win.cpp
        ../../corelib/kernel/qcoreapplication_win.cpp
        ../../corelib/plugin/qsystemlibrary.cpp
    LIBRARIES
        user32
        ole32
        advapi32
        shell32
        netapi32
)

extend_target(Bootstrap CONDITION APPLE_OSX
    SOURCES
        ../../corelib/global/qoperatingsystemversion_darwin.mm
        ../../corelib/io/qstandardpaths_mac.mm
        ../../corelib/kernel/qcore_foundation.mm
        ../../corelib/kernel/qcore_mac.cpp
        ../../corelib/kernel/qcore_mac_objc.mm
        ../../corelib/kernel/qcoreapplication_mac.cpp
    LIBRARIES
        ${FWCoreServices}
        ${FWFoundation}
)

extend_target(Bootstrap CONDITION APPLE_OSX AND APPLE_UIKIT
    LIBRARIES
        ${FWUIKit}
)

extend_target(Bootstrap CONDITION UNIX AND NOT APPLE_OSX
    SOURCES
        ../../corelib/io/qstandardpaths_unix.cpp
)

# special case: This will never get cross_compiled!
# extend_target(Bootstrap CONDITION cross_compile [...])
# extend_target(Bootstrap CONDITION NOT cross_compile [...])

extend_target(Bootstrap CONDITION WIN32 AND mingw
    LIBRARIES
        uuid
)

# special case:
qt_internal_add_link_flags_gc_sections(Bootstrap PUBLIC)
set_target_properties(Bootstrap PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

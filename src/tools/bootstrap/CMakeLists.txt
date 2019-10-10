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
        ../../corelib/kernel/qmetatype.cpp
        ../../corelib/kernel/qsharedmemory.cpp
        ../../corelib/kernel/qsystemerror.cpp
        ../../corelib/kernel/qsystemsemaphore.cpp
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
        ../../corelib/text/qbytearray.cpp
        ../../corelib/text/qbytearraymatcher.cpp
        ../../corelib/text/qlocale.cpp
        ../../corelib/text/qlocale_tools.cpp
        ../../corelib/text/qregexp.cpp
        ../../corelib/text/qstring.cpp
        ../../corelib/text/qstring_compat.cpp
        ../../corelib/text/qstringbuilder.cpp
        ../../corelib/text/qstringlist.cpp
        ../../corelib/text/qstringview.cpp
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
        ../../corelib/tools/qlist.cpp
        ../../corelib/tools/qmap.cpp
        ../../corelib/tools/qpoint.cpp
        ../../corelib/tools/qrect.cpp
        ../../corelib/tools/qringbuffer.cpp
        ../../corelib/tools/qsize.cpp
        ../../corelib/tools/qversionnumber.cpp
        ../../xml/dom/qdom.cpp
        ../../xml/sax/qxml.cpp
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
    PUBLIC_INCLUDE_DIRECTORIES  # special case
        $<TARGET_PROPERTY:CorePrivate,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:XmlPrivate,INTERFACE_INCLUDE_DIRECTORIES>
    PUBLIC_LIBRARIES  # special case
        ZLIB::ZLIB
        Qt::Platform
)

#### Keys ignored in scope 1:.:.:bootstrap.pro:<TRUE>:
# INSTALLS = "lib"
# MODULE_CONFIG = "gc_binaries"
# MODULE_INCNAME = "QtCore" "QtXml"
# _LOADED = "qt_module"
# _OPTION = "host_build"
# lib.CONFIG = "dummy_install"

## Scopes:
#####################################################################

extend_target(Bootstrap CONDITION UNIX
    SOURCES
        ../../corelib/io/qfilesystemengine_unix.cpp
        ../../corelib/io/qfilesystemiterator_unix.cpp
        ../../corelib/io/qfsfileengine_unix.cpp
        ../../corelib/kernel/qcore_unix.cpp
        ../../corelib/kernel/qsharedmemory_posix.cpp
        ../../corelib/kernel/qsharedmemory_systemv.cpp
        ../../corelib/kernel/qsharedmemory_unix.cpp
        ../../corelib/kernel/qsystemsemaphore_posix.cpp
        ../../corelib/kernel/qsystemsemaphore_systemv.cpp
        ../../corelib/kernel/qsystemsemaphore_unix.cpp
)

extend_target(Bootstrap CONDITION WIN32
    SOURCES
        ../../corelib/global/qoperatingsystemversion_win.cpp
        ../../corelib/io/qfilesystemengine_win.cpp
        ../../corelib/io/qfilesystemiterator_win.cpp
        ../../corelib/io/qfsfileengine_win.cpp
        ../../corelib/io/qstandardpaths_win.cpp
        ../../corelib/kernel/qcoreapplication_win.cpp
        ../../corelib/kernel/qsharedmemory_win.cpp
        ../../corelib/kernel/qsystemsemaphore_win.cpp
        ../../corelib/plugin/qsystemlibrary.cpp
    PUBLIC_LIBRARIES
        advapi32
        netapi32
        ole32
        shell32
        user32
)

extend_target(Bootstrap CONDITION APPLE
    SOURCES
        ../../corelib/global/qoperatingsystemversion_darwin.mm
        ../../corelib/kernel/qcore_foundation.mm
        ../../corelib/kernel/qcore_mac.cpp
        ../../corelib/kernel/qcore_mac_objc.mm
        ../../corelib/kernel/qcoreapplication_mac.cpp
    PUBLIC_LIBRARIES
        ${FWFoundation}
)

extend_target(Bootstrap CONDITION APPLE_OSX
    SOURCES
        ../../corelib/io/qstandardpaths_mac.mm
    LIBRARIES
        ${FWCoreServices}
)

extend_target(Bootstrap CONDITION APPLE_UIKIT
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
    PUBLIC_LIBRARIES
        uuid
)

# special case:
qt_internal_add_link_flags_gc_sections(Bootstrap PUBLIC)
set_target_properties(Bootstrap PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
qt_internal_add_target_aliases(Bootstrap)

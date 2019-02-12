
if(WIN32)
    set(os_sources
        ../../corelib/global/qoperatingsystemversion_win.cpp
        ../../corelib/io/qfilesystemengine_win.cpp
        ../../corelib/io/qfilesystemiterator_win.cpp
        ../../corelib/io/qfsfileengine_win.cpp
        ../../corelib/kernel/qcoreapplication_win.cpp
        ../../corelib/plugin/qsystemlibrary.cpp
        ../../corelib/io/qstandardpaths_win.cpp
        )
    set(os_libs user32 ole32 advapi32 shell32 netapi32)
else()
    set(os_sources
        ../../corelib/kernel/qcore_unix.cpp
        ../../corelib/io/qfilesystemengine_unix.cpp
        ../../corelib/io/qfilesystemiterator_unix.cpp
        ../../corelib/io/qfsfileengine_unix.cpp
    )

    if(APPLE)
        list(APPEND os_sources
            ../../corelib/global/qoperatingsystemversion_darwin.mm
            ../../corelib/io/qstandardpaths_mac.mm
            ../../corelib/kernel/qcoreapplication_mac.cpp
            ../../corelib/kernel/qcore_mac.cpp
            ../../corelib/kernel/qcore_mac_objc.mm
            ../../corelib/kernel/qcore_foundation.mm
            ../../corelib/io/qurl.cpp
        )
    else()
        list(APPEND os_sources ../../corelib/io/qstandardpaths_unix.cpp)
    endif()
    set(os_libs "")
endif()

add_library(Bootstrap STATIC
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
    ../../corelib/io/qfileinfo.cpp
    ../../corelib/io/qfilesystementry.cpp
    ../../corelib/io/qfilesystemengine.cpp
    ../../corelib/io/qfsfileengine.cpp
    ../../corelib/io/qfsfileengine_iterator.cpp
    ../../corelib/io/qiodevice.cpp
    ../../corelib/io/qfiledevice.cpp
    ../../corelib/io/qresource.cpp
    ../../corelib/io/qtemporarydir.cpp
    ../../corelib/io/qtemporaryfile.cpp
    ../../corelib/io/qsavefile.cpp
    ../../corelib/io/qstandardpaths.cpp
    ../../corelib/io/qloggingcategory.cpp
    ../../corelib/io/qloggingregistry.cpp
    ../../corelib/kernel/qcoreapplication.cpp
    ../../corelib/kernel/qcoreglobaldata.cpp
    ../../corelib/kernel/qmetatype.cpp
    ../../corelib/kernel/qvariant.cpp
    ../../corelib/kernel/qsystemerror.cpp
    ../../corelib/plugin/quuid.cpp
    ../../corelib/serialization/qdatastream.cpp
    ../../corelib/serialization/qjson.cpp
    ../../corelib/serialization/qjsondocument.cpp
    ../../corelib/serialization/qjsonobject.cpp
    ../../corelib/serialization/qjsonarray.cpp
    ../../corelib/serialization/qjsonvalue.cpp
    ../../corelib/serialization/qjsonparser.cpp
    ../../corelib/serialization/qjsonwriter.cpp
    ../../corelib/serialization/qtextstream.cpp
    ../../corelib/serialization/qxmlutils.cpp
    ../../corelib/serialization/qxmlstream.cpp
    ../../corelib/tools/qbitarray.cpp
    ../../corelib/tools/qbytearray.cpp
    ../../corelib/tools/qarraydata.cpp
    ../../corelib/tools/qbytearraymatcher.cpp
    ../../corelib/tools/qcommandlineparser.cpp
    ../../corelib/tools/qcommandlineoption.cpp
    ../../corelib/tools/qcryptographichash.cpp
    ../../corelib/tools/qdatetime.cpp
    ../../corelib/tools/qhash.cpp
    ../../corelib/tools/qlist.cpp
    ../../corelib/tools/qlinkedlist.cpp
    ../../corelib/tools/qlocale.cpp
    ../../corelib/tools/qlocale_tools.cpp
    ../../corelib/tools/qmap.cpp
    ../../corelib/tools/qregexp.cpp
    ../../corelib/tools/qringbuffer.cpp
    ../../corelib/tools/qpoint.cpp
    ../../corelib/tools/qrect.cpp
    ../../corelib/tools/qsize.cpp
    ../../corelib/tools/qline.cpp
    ../../corelib/tools/qstring.cpp
    ../../corelib/tools/qstringbuilder.cpp
    ../../corelib/tools/qstring_compat.cpp
    ../../corelib/tools/qstringlist.cpp
    ../../corelib/tools/qversionnumber.cpp
    ../../corelib/tools/qvsnprintf.cpp
    ../../xml/dom/qdom.cpp
    ../../xml/sax/qxml.cpp
    ${os_sources}
)

add_library(Qt::Bootstrap ALIAS Bootstrap)
qt_internal_add_target_aliases(Bootstrap)

qt_ensure_perl()
foreach(module QtCore QtXml)
    execute_process(COMMAND "${HOST_PERL}" -w "${PROJECT_SOURCE_DIR}/bin/syncqt.pl" -quiet -minimal -module "${module}" -version "${PROJECT_VERSION}" -outdir "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}")
    target_include_directories(Bootstrap PUBLIC
        "${PROJECT_BINARY_DIR}/include/${module}"
        "${PROJECT_BINARY_DIR}/include/${module}/${PROJECT_VERSION}"
        "${PROJECT_BINARY_DIR}/include/${module}/${PROJECT_VERSION}/${module}")
endforeach()

target_compile_definitions(Bootstrap PUBLIC
    QT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    QT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    QT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    QT_VERSION_STR="${PROJECT_VERSION}"
    QT_BOOTSTRAPPED
    QT_NO_CAST_ASCII
    QT_NO_FOREACH
    QT_NO_CAST_FROM_ASCII
)
target_link_libraries(Bootstrap PUBLIC Qt::Platform ${os_libs})
target_include_directories(Bootstrap PUBLIC "${PROJECT_BINARY_DIR}/include")
target_link_libraries(Bootstrap PRIVATE ZLIB::ZLIB)

### fixme: Expose as some cmake equivalent of a reusable qmake mkspecs/features/gc_binaries.prf.
if(APPLE)
    target_link_libraries(Bootstrap INTERFACE "-Wl,-dead_strip")
endif()

if (APPLE)
    find_library(FWFoundation Foundation)
    find_library(FWCoreServices CoreServices)
    target_link_libraries(Bootstrap INTERFACE ${FWFoundation})
    target_link_libraries(Bootstrap PRIVATE ${FWCoreServices})
endif()

find_package(Threads)
find_package(WrapDoubleConversion REQUIRED)

find_library(FWAppKit AppKit)
find_library(FWApplicationServices ApplicationServices)
find_library(FWCoreFoundation CoreFoundation)
find_library(FWCoreServices CoreServices)
find_library(FWDiskArbitration DiskArbitration)
find_library(FWFoundation Foundation)
find_library(FWIOKit IOKit)
find_library(FWMobileCoreServices MobileCoreServices)
find_library(FWSecurity Security)
find_library(FWUIKit UIKit)
find_library(FWWatchKit WatchKit)

## Evaluation helper target:
# FIXME: How does this work?
add_library(Qt_Evaluation INTERFACE)
target_sources(Qt_Evaluation INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/kernel/qtcore_eval.cpp")
target_include_directories(Qt_Evaluation INTERFACE global)

#####################################################################
## Core Module:
#####################################################################

add_qt_module(Core
    SOURCES
        animation/qabstractanimation.cpp animation/qabstractanimation.h animation/qabstractanimation_p.h
        animation/qanimationgroup.cpp animation/qanimationgroup.h animation/qanimationgroup_p.h
        animation/qparallelanimationgroup.cpp animation/qparallelanimationgroup.h animation/qparallelanimationgroup_p.h
        animation/qpauseanimation.cpp animation/qpauseanimation.h
        animation/qpropertyanimation.cpp animation/qpropertyanimation.h animation/qpropertyanimation_p.h
        animation/qsequentialanimationgroup.cpp animation/qsequentialanimationgroup.h animation/qsequentialanimationgroup_p.h
        animation/qvariantanimation.cpp animation/qvariantanimation.h animation/qvariantanimation_p.h
        codecs/qisciicodec.cpp codecs/qisciicodec_p.h
        codecs/qlatincodec.cpp codecs/qlatincodec_p.h
        codecs/qsimplecodec.cpp codecs/qsimplecodec_p.h
        codecs/qtextcodec.cpp codecs/qtextcodec.h codecs/qtextcodec_p.h
        codecs/qtsciicodec.cpp codecs/qtsciicodec_p.h
        codecs/qutfcodec.cpp codecs/qutfcodec_p.h
        global/archdetect.cpp
        global/qcompilerdetection.h
        global/qendian.cpp global/qendian.h global/qendian_p.h
        global/qflags.h
        global/qfloat16.cpp global/qfloat16.h global/qfloat16_p.h
        global/qglobal.cpp global/qglobal.h
        global/qglobalstatic.h
        global/qhooks.cpp global/qhooks_p.h
        global/qisenum.h
        global/qlibraryinfo.cpp global/qlibraryinfo.h
        global/qlogging.cpp global/qlogging.h
        global/qmalloc.cpp
        global/qnumeric.cpp global/qnumeric.h global/qnumeric_p.h
        global/qoperatingsystemversion.cpp global/qoperatingsystemversion.h global/qoperatingsystemversion_p.h
        global/qprocessordetection.h
        global/qrandom.cpp global/qrandom.h global/qrandom_p.h
        global/qsysinfo.h
        global/qsystemdetection.h
        global/qtypeinfo.h
        global/qtypetraits.h
        global/qversiontagging.cpp global/qversiontagging.h
        io/qabstractfileengine.cpp io/qabstractfileengine_p.h
        io/qbuffer.cpp io/qbuffer.h
        io/qdataurl.cpp io/qdataurl_p.h
        io/qdebug.cpp io/qdebug.h io/qdebug_p.h
        io/qdir.cpp io/qdir.h io/qdir_p.h
        io/qdiriterator.cpp io/qdiriterator.h
        io/qfile.cpp io/qfile.h
        io/qfiledevice.cpp io/qfiledevice.h io/qfiledevice_p.h
        io/qfileinfo.cpp io/qfileinfo.h io/qfileinfo_p.h
        io/qfileselector.cpp io/qfileselector.h io/qfileselector_p.h
        io/qfilesystemengine.cpp io/qfilesystemengine_p.h
        io/qfilesystementry.cpp io/qfilesystementry_p.h
        io/qfilesystemiterator_p.h
        io/qfilesystemmetadata_p.h
        io/qfilesystemwatcher.cpp io/qfilesystemwatcher.h io/qfilesystemwatcher_p.h
        io/qfilesystemwatcher_polling.cpp io/qfilesystemwatcher_polling_p.h
        io/qfsfileengine.cpp io/qfsfileengine_p.h
        io/qfsfileengine_iterator.cpp io/qfsfileengine_iterator_p.h
        io/qiodevice.cpp io/qiodevice.h io/qiodevice_p.h
        io/qipaddress.cpp io/qipaddress_p.h
        io/qlockfile.cpp io/qlockfile.h io/qlockfile_p.h
        io/qloggingcategory.cpp io/qloggingcategory.h
        io/qloggingregistry.cpp io/qloggingregistry_p.h
        io/qnoncontiguousbytedevice.cpp io/qnoncontiguousbytedevice_p.h
        io/qresource.cpp io/qresource_p.h
        io/qresource_iterator.cpp io/qresource_iterator_p.h
        io/qsavefile.cpp io/qsavefile.h
        io/qsettings.cpp io/qsettings.h io/qsettings_p.h
        io/qstandardpaths.cpp io/qstandardpaths.h
        io/qstorageinfo.cpp io/qstorageinfo.h io/qstorageinfo_p.h
        io/qtemporarydir.cpp io/qtemporarydir.h
        io/qtemporaryfile.cpp io/qtemporaryfile.h io/qtemporaryfile_p.h
        io/qtldurl.cpp io/qtldurl_p.h
        io/qurl.cpp io/qurl.h io/qurl_p.h
        io/qurlidna.cpp
        io/qurlquery.cpp io/qurlquery.h
        io/qurlrecode.cpp
        io/qurltlds_p.h
        kernel/qabstracteventdispatcher.cpp kernel/qabstracteventdispatcher.h kernel/qabstracteventdispatcher_p.h
        kernel/qabstractnativeeventfilter.cpp kernel/qabstractnativeeventfilter.h
        kernel/qbasictimer.cpp kernel/qbasictimer.h
        kernel/qcoreapplication.cpp kernel/qcoreapplication.h kernel/qcoreapplication_p.h
        kernel/qcorecmdlineargs_p.h
        kernel/qcoreevent.cpp kernel/qcoreevent.h
        kernel/qcoreglobaldata.cpp kernel/qcoreglobaldata_p.h
        kernel/qdeadlinetimer.cpp kernel/qdeadlinetimer.h kernel/qdeadlinetimer_p.h
        kernel/qelapsedtimer.cpp kernel/qelapsedtimer.h
        kernel/qeventloop.cpp kernel/qeventloop.h
        kernel/qfunctions_p.h
        kernel/qmath.cpp kernel/qmath.h
        kernel/qmetaobject.cpp kernel/qmetaobject.h kernel/qmetaobject_p.h
        kernel/qmetaobject_moc_p.h
        kernel/qmetaobjectbuilder.cpp kernel/qmetaobjectbuilder_p.h
        kernel/qmetatype.cpp kernel/qmetatype.h kernel/qmetatype_p.h
        kernel/qmetatypeswitcher_p.h
        kernel/qmimedata.cpp kernel/qmimedata.h
        kernel/qobject_impl.h
        kernel/qobjectcleanuphandler.cpp kernel/qobjectcleanuphandler.h
        kernel/qobjectdefs.h
        kernel/qobjectdefs_impl.h
        kernel/qpointer.cpp kernel/qpointer.h
        kernel/qsharedmemory.cpp kernel/qsharedmemory.h kernel/qsharedmemory_p.h
        kernel/qsignalmapper.cpp kernel/qsignalmapper.h
        kernel/qsocketnotifier.cpp kernel/qsocketnotifier.h
        kernel/qsystemerror.cpp kernel/qsystemerror_p.h
        kernel/qsystemsemaphore.cpp kernel/qsystemsemaphore.h kernel/qsystemsemaphore_p.h
        kernel/qtestsupport_core.cpp kernel/qtestsupport_core.h
        kernel/qtimer.cpp kernel/qtimer.h
        kernel/qtranslator.cpp kernel/qtranslator.h kernel/qtranslator_p.h
        kernel/qvariant.cpp kernel/qvariant.h kernel/qvariant_p.h
        plugin/qfactoryinterface.cpp plugin/qfactoryinterface.h
        plugin/qfactoryloader.cpp plugin/qfactoryloader_p.h
        plugin/qplugin.h plugin/qplugin_p.h
        plugin/qpluginloader.cpp plugin/qpluginloader.h
        plugin/quuid.cpp plugin/quuid.h
        serialization/qcborarray.h
        serialization/qcborcommon.h
        serialization/qcbordiagnostic.cpp
        serialization/qcbormap.h
        serialization/qcborstream.cpp serialization/qcborstream.h
        serialization/qcborvalue.cpp serialization/qcborvalue.h serialization/qcborvalue_p.h
        serialization/qdatastream.cpp serialization/qdatastream.h serialization/qdatastream_p.h
        serialization/qjson.cpp serialization/qjson_p.h
        serialization/qjsonarray.cpp serialization/qjsonarray.h
        serialization/qjsoncbor.cpp
        serialization/qjsondocument.cpp serialization/qjsondocument.h
        serialization/qjsonobject.cpp serialization/qjsonobject.h
        serialization/qjsonparser.cpp serialization/qjsonparser_p.h
        serialization/qjsonvalue.cpp serialization/qjsonvalue.h
        serialization/qjsonwriter.cpp serialization/qjsonwriter_p.h
        serialization/qtextstream.cpp serialization/qtextstream.h serialization/qtextstream_p.h
        serialization/qxmlstream.cpp serialization/qxmlstream.h serialization/qxmlstream_p.h
        serialization/qxmlutils.cpp serialization/qxmlutils_p.h
        thread/qmutex.h
        thread/qreadwritelock.h
        thread/qrunnable.cpp thread/qrunnable.h
        thread/qthread.cpp thread/qthread.h
        thread/qthreadstorage.h
        thread/qwaitcondition.h
        tools/qalgorithms.h
        tools/qarraydata.cpp tools/qarraydata.h
        tools/qarraydataops.h
        tools/qarraydatapointer.h
        tools/qbitarray.cpp tools/qbitarray.h
        tools/qbytearray.cpp tools/qbytearray.h tools/qbytearray_p.h
        tools/qbytearraylist.cpp tools/qbytearraylist.h
        tools/qbytearraymatcher.cpp tools/qbytearraymatcher.h
        tools/qbytedata_p.h
        tools/qcache.h
        tools/qchar.h
        tools/qcollator.cpp tools/qcollator.h tools/qcollator_p.h
        tools/qcontainerfwd.h
        tools/qcontiguouscache.cpp tools/qcontiguouscache.h
        tools/qcryptographichash.cpp tools/qcryptographichash.h
        tools/qdatetime.cpp tools/qdatetime.h tools/qdatetime_p.h
        tools/qdoublescanprint_p.h
        tools/qeasingcurve.cpp tools/qeasingcurve.h
        tools/qfreelist.cpp tools/qfreelist_p.h
        tools/qharfbuzz.cpp
        tools/qhash.cpp tools/qhash.h
        tools/qhashfunctions.h
        tools/qiterator.h
        tools/qline.cpp tools/qline.h
        tools/qlinkedlist.cpp tools/qlinkedlist.h
        tools/qlist.cpp tools/qlist.h
        tools/qlocale.cpp tools/qlocale.h tools/qlocale_p.h
        tools/qlocale_data_p.h
        tools/qlocale_tools.cpp tools/qlocale_tools_p.h
        tools/qmakearray_p.h
        tools/qmap.cpp tools/qmap.h
        tools/qmargins.cpp tools/qmargins.h
        tools/qmessageauthenticationcode.cpp tools/qmessageauthenticationcode.h
        tools/qoffsetstringarray_p.h
        tools/qpair.h
        tools/qpoint.cpp tools/qpoint.h
        tools/qqueue.h
        tools/qrect.cpp tools/qrect.h
        tools/qrefcount.cpp tools/qrefcount.h
        tools/qregexp.cpp tools/qregexp.h
        tools/qringbuffer.cpp tools/qringbuffer_p.h
        tools/qscopedpointer.h tools/qscopedpointer_p.h
        tools/qscopedvaluerollback.h
        tools/qscopeguard.h
        tools/qset.h
        tools/qshareddata.cpp tools/qshareddata.h
        tools/qsharedpointer.cpp tools/qsharedpointer.h
        tools/qsharedpointer_impl.h
        tools/qsimd.cpp tools/qsimd_p.h
        tools/qsize.cpp tools/qsize.h
        tools/qstack.h
        tools/qstring.cpp tools/qstring.h
        tools/qstringalgorithms.h tools/qstringalgorithms_p.h
        tools/qstringbuilder.cpp tools/qstringbuilder.h
        tools/qstringiterator_p.h
        tools/qstringlist.cpp tools/qstringlist.h
        tools/qstringliteral.h
        tools/qstringmatcher.h
        tools/qstringview.cpp tools/qstringview.h
        tools/qtextboundaryfinder.cpp tools/qtextboundaryfinder.h
        tools/qtimeline.cpp tools/qtimeline.h
        tools/qtools_p.h
        tools/qunicodetables_p.h
        tools/qunicodetools.cpp tools/qunicodetools_p.h
        tools/qvarlengtharray.h
        tools/qvector.h
        tools/qversionnumber.cpp tools/qversionnumber.h
        tools/qvsnprintf.cpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_BINARY_DIR}/global # For qconfig.cpp
    LIBRARIES
        WrapDoubleConversion
        tinycbor Threads::Threads ZLIB::ZLIB
        QtHarfBuzz
        Qt::GlobalConfigPrivate
    PUBLIC_LIBRARIES Qt::Platform
    DEFINES
        QT_NO_USING_NAMESPACE
        QT_NO_FOREACH
)

add_dependencies(Core Qt::moc)
add_dependencies(Core Qt::rcc)
add_dependencies(CorePrivate Qt::moc)
add_dependencies(CorePrivate Qt::rcc)

configure_file(global/qconfig.cpp.in global/qconfig.cpp)

# Handle QObject: Automoc does not work for this as it would
# require to spill internals into users:
add_library(Core_qobject OBJECT)
qt_manual_moc(qobject_moc_files kernel/qobject.h global/qnamespace.h)
set_source_files_properties(${qobject_moc_files} PROPERTIES HEADER_FILE_ONLY ON)
target_sources(Core_qobject PRIVATE
    global/qnamespace.h
    kernel/qobject.cpp kernel/qobject.h kernel/qobject_p.h ${qobject_moc_files})
set_target_properties(Core_qobject PROPERTIES
    COMPILE_OPTIONS $<TARGET_PROPERTY:Core,COMPILE_OPTIONS>
    COMPILE_DEFINITIONS $<TARGET_PROPERTY:Core,COMPILE_DEFINITIONS>
    INCLUDE_DIRECTORIES $<TARGET_PROPERTY:Core,INCLUDE_DIRECTORIES>
)
target_include_directories(Core_qobject PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/kernel") # for moc_qobject.cpp
target_link_libraries(Core_qobject PRIVATE Qt::Platform Qt::GlobalConfig)
target_link_libraries(Core PRIVATE Core_qobject)

set_property(TARGET Core APPEND PROPERTY PUBLIC_HEADER "${CMAKE_CURRENT_BINARY_DIR}/global/qconfig.h")
set_property(TARGET Core APPEND PROPERTY PRIVATE_HEADER "${CMAKE_CURRENT_BINARY_DIR}/global/qconfig_p.h")

# FIXME: tools still have a lot of special stuff that is not ported!

if (LINUX AND NOT CMAKE_CROSSCOMPILING AND BUILD_SHARED_LIBS)
    if (NOT DEFINED ELF_INTERPRETER)
        execute_process(COMMAND ${CMAKE_COMMAND} -E env LC_ALL=C readelf -l /bin/ls
                        RESULT_VARIABLE readelf_ok
                        OUTPUT_VARIABLE readelf_output
                       )
        if ("${readelf_ok}" STREQUAL "0" AND "${readelf_output}" MATCHES "program interpreter: (.*)]")
            set(ELF_INTERPRETER "${CMAKE_MATCH_1}" CACHE INTERNAL "ELF interpreter location")
        else
            set(ELF_INTERPRETER "" CACHE INTERNAL "ELF interpreter location")
        endif()
    endif()
    if (ELF_INTERPRETER)
        qt_internal_add_link_flags(Core "-Wl,-e,qt_core_boilerplate")
        target_compile_definitions(Core PRIVATE ELF_INTERPRETER="${ELF_INTERPRETER}")
    endif()
endif()


### Platform support:
#####################################################################

# Windows

### All Windows:
extend_target(Core CONDITION WIN32
    SOURCES
        io/qfilesystemengine_win.cpp
        io/qfilesystemwatcher_win.cpp io/qfilesystemwatcher_win_p.h
        io/qfsfileengine_win.cpp
        io/qlockfile_win.cpp
        kernel/qcoreapplication_win.cpp
        kernel/qelapsedtimer_win.cpp
        kernel/qsharedmemory_win.cpp
        kernel/qsystemsemaphore_win.cpp
        kernel/qwineventnotifier.cpp
        kernel/qwineventnotifier.h
        kernel/qwineventnotifier_p.h
        plugin/qsystemlibrary.cpp
        plugin/qsystemlibrary_p.h
        thread/qatomic_msvc.h
        thread/qthread_win.cpp
        tools/qlocale_win.cpp
        global/qoperatingsystemversion_win.cpp
    LIBRARIES kernel32 user32 shell32 uuid ole32 advapi32 winmm version ws2_32
)

### Classic windows:
extend_target(Core CONDITION WIN32 AND NOT WINRT
    SOURCES
        io/qsettings_win.cpp
        io/qstandardpaths_win.cpp
        io/qstorageinfo_win.cpp
        io/qwindowspipereader.cpp
        io/qwindowspipereader_p.h
        io/qwindowspipewriter.cpp
        io/qwindowspipewriter_p.h
        kernel/qeventdispatcher_win.cpp kernel/qeventdispatcher_win_p.h
    LIBRARIES mpr netapi32 userenv
)

### WinRT
extend_target(Core CONDITION WIN32 AND WINRT
    SOURCES
        io/qsettings_winrt.cpp
        io/qstandardpaths_winrt.cpp
        io/qstorageinfo_stub.cpp
        kernel/qeventdispatcher_winrt.cpp kernel/qeventdispatcher_winrt_p.h
        kernel/qfunctions_fake_env_p.h
        kernel/qfunctions_winrt.cpp kernel/qfunctions_winrt.h
)

### All Apple:
extend_target(Core CONDITION APPLE
    SOURCES
        io/qsettings_mac.cpp
        io/qstandardpaths_mac.mm
        io/qstorageinfo_mac.cpp
        kernel/qcfsocketnotifier.cpp kernel/qcfsocketnotifier_p.h
        kernel/qcoreapplication_mac.cpp
        kernel/qcore_foundation.mm
        kernel/qcore_mac_objc.mm
        kernel/qcore_mac_objc.mm kernel/qcore_mac.cpp kernel/qcore_mac_p.h
        kernel/qelapsedtimer_mac.cpp
        kernel/qeventdispatcher_cf.mm kernel/qeventdispatcher_cf_p.h
        global/qoperatingsystemversion_darwin.mm
    LIBRARIES ${FWCoreFoundation} ${FWFoundation}
)

### Apple desktop machines:
extend_target(Core CONDITION APPLE_OSX
    SOURCES
        io/qfilesystemwatcher_fsevents.mm io/qfilesystemwatcher_fsevents_p.h
        tools/qlocale_mac.mm
    LIBRARIES
        ${FWCoreServices} ${FWAppKit} ${FWSecurity}
        ${FWApplicationServices} ${FWDiskArbitration} ${FWIOKit}
)

### Apple mobile machines:
extend_target(Core CONDITION APPLE AND NOT APPLE_OSX
    LIBRARIES ${FWMobileCoreServices}
)

extend_target(Core CONDITION APPLE_WATCHOS LIBRARIES ${FWWatchKit})
extend_target(Core CONDITION APPLE_TVOS LIBRARIES ${FWUIKit})

extend_target(Core CONDITION INTEGRITY
    SOURCES tools/qlocale_unix.cpp
    LIBRARIES posix ivfs socket net shm_client
)

extend_target(Core CONDITION UNIX
    SOURCES
        io/qfilesystemengine_unix.cpp
        io/qfsfileengine_unix.cpp
        io/qlockfile_unix.cpp
        thread/qthread_unix.cpp
)

extend_target(Core CONDITION UNIX AND NOT APPLE
    SOURCES
        tools/qlocale_unix.cpp
)

extend_target(Core CONDITION ANDROID
    SOURCES
        io/qstandardpaths_android.cpp
        io/qstorageinfo_unix.cpp
        kernel/qjni.cpp kernel/qjni_p.h
        kernel/qjnihelpers.cpp kernel/qjnihelpers_p.h
        kernel/qjnionload.cpp
)

extend_target(Core CONDITION HAIKU
    SOURCES
        io/qstandardpaths_haiku.cpp
        io/qstorageinfo_unix.cpp
    LIBRARIES be
)

extend_target(Core CONDITION BSD
    SOURCES io/qfilesystemwatcher_kqueue.cpp io/qfilesystemwatcher_kqueue_p.h
)

extend_target(Core CONDITION VXWORKS
    SOURCES kernel/qfunctions_vxworks.cpp kernel/qfunctions_vxworks.h
)

extend_target(Core CONDITION NACL
    SOURCES kernel/qfunctions_nacl.cpp kernel/qfunctions_nacl.h
)

extend_target(Core CONDITION NOT WIN32 AND NOT APPLE AND NOT ANDROID AND NOT HAIKU
    SOURCES
        io/qstandardpaths_unix.cpp
        io/qstorageinfo_unix.cpp
)

### standardpaths, storageinfo and settings:
if (WIN32)
    if (WINRT)
        target_sources(Core PRIVATE
            io/qsettings_winrt.cpp
            io/qstandardpaths_winrt.cpp
            io/qstorageinfo_stub.cpp
        )
    else()
        target_sources(Core PRIVATE
            io/qsettings_win.cpp
            io/qstandardpaths_win.cpp
            io/qstorageinfo_win.cpp
        )
    endif()
elseif (UNIX)
    if (NOT INTEGRITY AND NOT UIKIT)
        target_sources(Core PRIVATE io/forkfd_qt.cpp)
        target_include_directories(Core PRIVATE ../3rdparty/forkfd)
    endif()
    if (NOT NACL AND APPLE)
        target_sources(Core PRIVATE io/qsettings_mac.cpp)
    endif()
    if (APPLE)
        target_sources(Core PRIVATE
            io/qstandardpaths_mac.mm
            io/qstorageinfo_mac.cpp
        )
    elseif(ANDROID AND NOT ANDROID_EMBEDDED)
        target_sources(Core PRIVATE
            io/qstandardpaths_android.cpp
            io/qstorageinfo_unix.cpp
        )
    elseif(HAIKU)
        target_sources(Core PRIVATE
            io/qstandardpaths_haiku.cpp
            io/qstorageinfo_unix.cpp
        )
    else()
        target_sources(Core PRIVATE
            io/qstandardpaths_unix.cpp
            io/qstorageinfo_unix.cpp
        )
    endif()
endif()

### kernel stuff:
if (UNIX OR INTEGRITY)
    extend_target(Core SOURCES
            kernel/qcore_unix.cpp
            kernel/qcore_unix_p.h
            kernel/qeventdispatcher_unix.cpp
            kernel/qeventdispatcher_unix_p.h
            kernel/qpoll_p.h
            kernel/qtimerinfo_unix.cpp
            kernel/qtimerinfo_unix_p.h
    )
    if (NOT APPLE OR NACL)
        target_sources(Core PRIVATE kernel/qelapsedtimer_unix.cpp)
    endif()

    if (ANDROID)
        target_sources(Core PRIVATE
            kernel/qsharedmemory_android.cpp
            kernel/qsystemsemaphore_android.cpp
        )
    else()
        target_sources(Core PRIVATE
            kernel/qsharedmemory_posix.cpp
            kernel/qsharedmemory_systemv.cpp
            kernel/qsharedmemory_unix.cpp
            kernel/qsystemsemaphore_posix.cpp
            kernel/qsystemsemaphore_systemv.cpp
            kernel/qsystemsemaphore_unix.cpp
        )
    endif()

    # FIXME: Implement this!
    # This is needed by QMetaType::typeName array implementation
    #integrity: QMAKE_CXXFLAGS += --pending_instantiations=128
elseif(NOT WIN32 AND NOT UNIX AND NOT APPLE)
    target_sources(Core PRIVATE kernel/qelapsedtimer_generic.cpp)
endif()



### Feature support:
#####################################################################

## inotify:
extend_target(Core CONDITION QT_FEATURE_inotify
    SOURCES
        io/qfilesystemwatcher_inotify.cpp io/qfilesystemwatcher_inotify_p.h
)

## qqnx_pps:
extend_target(Core CONDITION QNX AND QT_FEATURE_qqnx_pps
    SOURCES
        kernel/qppsattribute.cpp kernel/qppsattribute_p.h
        kernel/qppsattributeprivate_p.h
        kernel/qppsobject.cpp kernel/qppsobject_p.h
        kernel/qppsobjectprivate_p.h
    LIBRARIES PPS::PPS
)

## clock_gettime:
extend_target(Core CONDITION UNIX AND QT_FEATURE_clock_gettime
    LIBRARIES WrapRt
)

## glib:
extend_target(Core CONDITION QT_FEATURE_glib
    SOURCES
        kernel/qeventdispatcher_glib.cpp kernel/qeventdispatcher_glib_p.h
    LIBRARIES GLIB2::GLIB2
)

## poll_select:
extend_target(Core CONDITION UNIX AND QT_FEATURE_poll_select
    SOURCES kernel/qpoll.cpp
)

## thread:
extend_target(Core CONDITION QT_FEATURE_thread
    SOURCES
        thread/qatomic_bootstrap.h
        thread/qatomic.cpp thread/qatomic.h
        thread/qatomic_cxx11.h
        thread/qbasicatomic.h
        thread/qfutex_p.h
        thread/qgenericatomic.h
        thread/qmutex.cpp thread/qmutex_p.h
        thread/qmutexpool.cpp thread/qmutexpool_p.h
        thread/qorderedmutexlocker_p.h
        thread/qreadwritelock.cpp thread/qreadwritelock_p.h
        thread/qsemaphore.cpp thread/qsemaphore.h
        thread/qthread_p.h
        thread/qthreadpool.cpp thread/qthreadpool.h thread/qthreadpool_p.h
        thread/qthreadstorage.cpp
)

extend_target(Core CONDITION QT_FEATURE_thread AND WIN32
    SOURCES
        thread/qwaitcondition_win.cpp
)

extend_target(Core CONDITION QT_FEATURE_thread AND NOT WIN32
    SOURCES thread/qwaitcondition_unix.cpp
)

## future:
extend_target(Core CONDITION QT_FEATURE_future
    SOURCES
        thread/qexception.cpp thread/qexception.h
        thread/qfuture.h
        thread/qfutureinterface.cpp thread/qfutureinterface.h thread/qfutureinterface_p.h
        thread/qfuturesynchronizer.h
        thread/qfuturewatcher.cpp thread/qfuturewatcher.h thread/qfuturewatcher_p.h
        thread/qresultstore.cpp thread/qresultstore.h
)

## ICU:
extend_target(Core CONDITION QT_FEATURE_icu
    SOURCES
        codecs/qicucodec.cpp
        tools/qcollator_icu.cpp
        tools/qlocale_icu.cpp
    LIBRARIES ICU::i18n ICU::uc ICU::data
)

extend_target(Core CONDITION NOT QT_FEATURE_icu AND WIN32
    SOURCES tools/qcollator_win.cpp
)

extend_target(Core CONDITION NOT QT_FEATURE_icu AND APPLE
    SOURCES tools/qcollator_macx.cpp
)

extend_target(Core CONDITION NOT QT_FEATURE_icu AND NOT WIN32 AND NOT APPLE
    SOURCES tools/qcollator_posix.cpp
)

extend_target(Core CONDITION NOT QT_FEATURE_icu
    SOURCES
        codecs/qbig5codec.cpp codecs/qbig5codec_p.h
        codecs/qeucjpcodec.cpp codecs/qeucjpcodec_p.h
        codecs/qeuckrcodec.cpp codecs/qeuckrcodec_p.h
        codecs/qgb18030codec.cpp codecs/qgb18030codec_p.h
        codecs/qjiscodec.cpp codecs/qjiscodec_p.h
        codecs/qjpunicode.cpp
        codecs/qsjiscodec.cpp codecs/qsjiscodec_p.h
)

extend_target(Core CONDITION NOT QT_FEATURE_icu AND QT_FEATURE_iconv
    SOURCES codecs/qiconvcodec.cpp codecs/qiconvcodec_p.h
    LIBRARIES Iconv
)

extend_target(Core CONDITION NOT QT_FEATURE_icu AND WIN32
    SOURCES codecs/qwindowscodec.cpp codecs/qwindowscodec_p.h
)

## timezone:
extend_target(Core CONDITION QT_FEATURE_timezone
    SOURCES
        tools/qtimezone.cpp tools/qtimezone.h
        tools/qtimezoneprivate.cpp tools/qtimezoneprivate_p.h
        tools/qtimezoneprivate_data_p.h
)

extend_target(Core CONDITION QT_FEATURE_timezone AND WIN32
    SOURCES tools/qtimezoneprivate_win.cpp
)

extend_target(Core CONDITION QT_FEATURE_timezone AND APPLE
    SOURCES tools/qtimezoneprivate_mac.mm
)

extend_target(Core CONDITION QT_FEATURE_timezone AND ANDROID
    SOURCES tools/qtimezoneprivate_android.cpp
)

extend_target(Core CONDITION QT_FEATURE_timezone AND UNIX AND NOT APPLE
    SOURCES tools/qtimezoneprivate_tz.cpp
)

extend_target(Core CONDITION QT_FEATURE_timezone AND QT_FEATURE_icu AND NOT WIN32 AND NOT ANDROID AND NOT APPLE
    SOURCES tools/qtimezoneprivate_icu.cpp
)

## datetimeparser:
extend_target(Core CONDITION QT_FEATURE_datetimeparser
    SOURCES tools/qdatetimeparser.cpp tools/qdatetimeparser_p.h
)

## regularexpression:
extend_target(Core CONDITION QT_FEATURE_regularexpression
    SOURCES tools/qregularexpression.cpp tools/qregularexpression.h
    LIBRARIES PCRE2
)

## commandlineparser:
extend_target(Core CONDITION QT_FEATURE_commandlineparser
    SOURCES
        tools/qcommandlineoption.cpp tools/qcommandlineoption.h
        tools/qcommandlineparser.cpp tools/qcommandlineparser.h
)

## processenvironment:
extend_target(Core CONDITION QT_FEATURE_processenvironment
    SOURCES io/qprocess.cpp io/qprocess.h io/qprocess_p.h
)

extend_target(Core CONDITION QT_FEATURE_processenvironment AND WIN32 AND NOT WINRT
    SOURCES io/qprocess_win.cpp
)

extend_target(Core CONDITION QT_FEATURE_processenvironment AND APPLE
    SOURCES io/qprocess_darwin.mm
)

extend_target(Core CONDITION QT_FEATURE_processenvironment AND UNIX
    SOURCES io/qprocess_unix.cpp
)

## filesystemiterator:
extend_target(Core CONDITION QT_FEATURE_filesystemiterator AND WIN32
    SOURCES io/qfilesystemiterator_win.cpp
)

extend_target(Core CONDITION QT_FEATURE_filesystemiterator AND UNIX
    SOURCES io/qfilesystemiterator_unix.cpp
)

## itemmodel:
extend_target(Core CONDITION QT_FEATURE_itemmodel
    SOURCES
        itemmodels/qabstractitemmodel.cpp itemmodels/qabstractitemmodel.h itemmodels/qabstractitemmodel_p.h
        itemmodels/qitemselectionmodel.cpp itemmodels/qitemselectionmodel.h itemmodels/qitemselectionmodel_p.h
)

## proxymodel:
extend_target(Core CONDITION QT_FEATURE_proxymodel
    SOURCES itemmodels/qabstractproxymodel.cpp itemmodels/qabstractproxymodel.h itemmodels/qabstractproxymodel_p.h
)

## identityproxymodel:
extend_target(Core CONDITION QT_FEATURE_identityproxymodel
    SOURCES itemmodels/qidentityproxymodel.cpp itemmodels/qidentityproxymodel.h
)

## identityproxymodel:
extend_target(Core CONDITION QT_FEATURE_sortfilterproxymodel
    SOURCES itemmodels/qsortfilterproxymodel.cpp itemmodels/qsortfilterproxymodel.h
)

## stringlistmodel:
extend_target(Core CONDITION QT_FEATURE_stringlistmodel
    SOURCES itemmodels/qstringlistmodel.cpp itemmodels/qstringlistmodel.h
)

## library:
extend_target(Core CONDITION QT_FEATURE_library
    SOURCES
        plugin/qelfparser_p.cpp plugin/qelfparser_p.h
        plugin/qlibrary.cpp plugin/qlibrary.h plugin/qlibrary_p.h
        plugin/qmachparser.cpp plugin/qmachparser_p.h
    LIBRARIES ${CMAKE_DL_LIBS}
)

extend_target(Core CONDITION QT_FEATURE_library AND WIN32
    SOURCES plugin/qlibrary_win.cpp
)

extend_target(Core CONDITION QT_FEATURE_library AND UNIX
    SOURCES plugin/qlibrary_unix.cpp
)

## statemachine:
extend_target(Core CONDITION QT_FEATURE_statemachine
    SOURCES
        statemachine/qabstractstate.cpp statemachine/qabstractstate.h statemachine/qabstractstate_p.h
        statemachine/qabstracttransition.cpp statemachine/qabstracttransition.h statemachine/qabstracttransition_p.h
        statemachine/qfinalstate.cpp statemachine/qfinalstate.h statemachine/qfinalstate_p.h
        statemachine/qhistorystate.cpp statemachine/qhistorystate.h statemachine/qhistorystate_p.h
        statemachine/qsignaleventgenerator_p.h
        statemachine/qsignaltransition.cpp statemachine/qsignaltransition.h statemachine/qsignaltransition_p.h
        statemachine/qstate.cpp statemachine/qstate.h statemachine/qstate_p.h
        statemachine/qstatemachine.cpp statemachine/qstatemachine.h statemachine/qstatemachine_p.h
    INCLUDE_DIRECTORIES statemachine
)

# qeventtransition:
extend_target(Core CONDITION QT_FEATURE_qeventtransition
    SOURCES statemachine/qeventtransition.cpp statemachine/qeventtransition.h statemachine/qeventtransition_p.h
)

# mimetype:
extend_target(Core CONDITION QT_FEATURE_mimetype
    SOURCES
        mimetypes/mimetypes.qrc
        mimetypes/qmimedatabase.cpp mimetypes/qmimedatabase.h mimetypes/qmimedatabase_p.h
        mimetypes/qmimeglobpattern.cpp mimetypes/qmimeglobpattern_p.h
        mimetypes/qmimemagicrule.cpp mimetypes/qmimemagicrule_p.h
        mimetypes/qmimemagicrulematcher.cpp mimetypes/qmimemagicrulematcher_p.h
        mimetypes/qmimeprovider.cpp mimetypes/qmimeprovider_p.h
        mimetypes/qmimetype.cpp mimetypes/qmimetype.h mimetypes/qmimetype_p.h
        mimetypes/qmimetypeparser.cpp mimetypes/qmimetypeparser_p.h
    INCLUDE_DIRECTORIES mimetypes
)

# Enable Evaluation based on QT_EVAL variable being set from the outside:
extend_target(Core CONDITION QT_EVAL LIBRARIES Qt_Evaluation)

### Extra stuff:
#####################################################################


add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/global/qfloat16tables.cpp"
                   COMMAND qfloat16-tables
                   ARGS "${CMAKE_CURRENT_BINARY_DIR}/global/qfloat16tables.cpp"
                   DEPENDS global/qfloat16.h
                   COMMENT qfloat16-tables
                   VERBATIM)
target_sources(Core PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/global/qfloat16tables.cpp")

qt_create_tracepoints(Core qtcore.tracepoints)

# Remove QT_NO_CAST_TO_ASCII to ensure that the symbols are included in the library.
if(WIN32)
    get_target_property(defines Core COMPILE_DEFINITIONS)
    list(REMOVE_ITEM defines QT_NO_CAST_TO_ASCII)
    set_target_properties(Core PROPERTIES COMPILE_DEFINITIONS "${defines}")
endif()

add_qt_docs(./doc/qtcore.qdocconf)

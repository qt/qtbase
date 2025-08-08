# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(WrapPCRE2 MODULE PROVIDED_TARGETS WrapPCRE2::WrapPCRE2)
qt_internal_extend_sbom(WrapPCRE2::WrapPCRE2
    CPE_VENDOR "pcre"
    CPE_PRODUCT "pcre2"
    DOWNLOAD_LOCATION "https://github.com/PCRE2Project/pcre2"
)
qt_find_package(WrapZLIB MODULE PROVIDED_TARGETS WrapZLIB::WrapZLIB)
qt_internal_extend_sbom(WrapZLIB::WrapZLIB
    CPE_VENDOR "zlib"
    CPE_PRODUCT "zlib"
    DOWNLOAD_LOCATION "https://github.com/madler/zlib"
)

if(ANDROID)
    set(corelib_extra_cmake_files
        "${CMAKE_CURRENT_SOURCE_DIR}/${QT_CMAKE_EXPORT_NAMESPACE}AndroidMacros.cmake")
endif()
if(WASM)
    set(corelib_extra_cmake_files
        "${CMAKE_CURRENT_SOURCE_DIR}/${QT_CMAKE_EXPORT_NAMESPACE}WasmMacros.cmake")
endif()

#####################################################################
## Core Module:
#####################################################################

qt_internal_add_module(Core
    QMAKE_MODULE_CONFIG moc resources
    NO_GENERATE_METATYPES # metatypes are extracted manually below
    EXCEPTIONS
    SOURCES
        # Keep these .cpp files in the first and in the order they are so their
        # static initialization order is retained
        global/qsimd.cpp global/qsimd.h global/qsimd_p.h
        tools/qhash.cpp tools/qhash.h

        # Keep the rest alphabetical
        compat/removed_api.cpp
        global/archdetect.cpp
        global/qalloc.cpp global/qalloc.h
        global/qassert.cpp global/qassert.h
        global/qcheckedint_impl.h
        global/qcompare_impl.h
        global/qcompare.cpp global/qcompare.h
        global/qcomparehelpers.h
        global/qcompilerdetection.h
        global/qconstructormacros.h
        global/qcontainerinfo.h
        global/qdarwinhelpers.h
        global/qendian.cpp global/qendian.h global/qendian_p.h
        global/qexceptionhandling.h
        global/qexpected_p.h
        global/qflags.h
        global/qfloat16.cpp global/qfloat16.h
        global/qforeach.h
        global/qfunctionpointer.h
        global/qgettid_p.h
        global/qglobal.cpp global/qglobal.h global/qglobal_p.h
        global/qglobalstatic.h
        global/qhooks.cpp global/qhooks_p.h
        global/qlibraryinfo.cpp global/qlibraryinfo.h global/qlibraryinfo_p.h
        global/qlogging.cpp global/qlogging.h global/qlogging_p.h
        global/qmalloc.cpp global/qmalloc.h
        global/qminmax.h
        global/qnamespace.h # this header is specified on purpose so AUTOMOC processes it
        global/qnativeinterface.h global/qnativeinterface_p.h
        global/qnumeric.cpp global/qnumeric.h global/qnumeric_p.h
        global/qoperatingsystemversion.cpp global/qoperatingsystemversion.h global/qoperatingsystemversion_p.h
        global/qoverload.h
        global/qprocessordetection.h
        global/qrandom.cpp global/qrandom.h global/qrandom_p.h
        global/qstdlibdetection.h
        global/qswap.h
        global/qsysinfo.cpp global/qsysinfo.h
        global/qsystemdetection.h
        global/qtclasshelpermacros.h
        global/qtclasshelper_p.h
        global/qtconfiginclude.h
        global/qtconfigmacros.h
        global/qtcoreglobal.h global/qtcoreglobal_p.h
        global/qtdeprecationmarkers.h
        global/qtenvironmentvariables.cpp global/qtenvironmentvariables.h
        global/qtenvironmentvariables_p.h
        global/qtnoop.h
        global/qtpreprocessorsupport.h
        global/qtrace_p.h
        global/qtresource.h
        global/qtsymbolmacros.h
        global/qttranslation.h
        global/qttypetraits.h
        global/qtversionchecks.h
        global/qtversion.h
        global/qtypeinfo.h
        global/qtypes.cpp global/qtypes.h
        global/qvolatile_p.h
        global/q17memory.h
        global/q20algorithm.h
        global/q20chrono.h
        global/q20functional.h
        global/q20iterator.h
        global/q20map.h
        global/q20memory.h
        global/q20type_traits.h
        global/q20utility.h
        global/q20vector.h
        global/q23functional.h
        global/q23utility.cpp # remove once we have a user that tests this
        global/q23type_traits.h
        global/q23utility.h
        global/q26numeric.h
        global/qxpfunctional.h
        global/qxptype_traits.h
        global/qversiontagging.h
        ipc/qsharedmemory.cpp ipc/qsharedmemory.h ipc/qsharedmemory_p.h
        ipc/qsystemsemaphore.cpp ipc/qsystemsemaphore.h ipc/qsystemsemaphore_p.h
        ipc/qtipccommon.cpp ipc/qtipccommon.h ipc/qtipccommon_p.h
        io/qabstractfileengine.cpp io/qabstractfileengine_p.h
        io/qbuffer.cpp io/qbuffer.h
        io/qdataurl.cpp io/qdataurl_p.h
        io/qdebug.cpp io/qdebug.h io/qdebug_p.h
        io/qdir.cpp io/qdir.h io/qdir_p.h
        io/qdirlisting.cpp io/qdirlisting.h io/qdirentryinfo_p.h
        io/qdiriterator.cpp io/qdiriterator.h
        io/qfile.cpp io/qfile.h io/qfile_p.h
        io/qfiledevice.cpp io/qfiledevice.h io/qfiledevice_p.h
        io/qfileinfo.cpp io/qfileinfo.h io/qfileinfo_p.h
        io/qfileselector.cpp io/qfileselector.h io/qfileselector_p.h
        io/qfilesystemengine.cpp io/qfilesystemengine_p.h
        io/qfilesystementry.cpp io/qfilesystementry_p.h
        io/qfilesystemiterator_p.h
        io/qfilesystemmetadata_p.h
        io/qfsfileengine.cpp io/qfsfileengine_p.h
        io/qfsfileengine_iterator.cpp io/qfsfileengine_iterator_p.h
        io/qiodevice.cpp io/qiodevice.h io/qiodevice_p.h
        io/qiodevicebase.h
        io/qipaddress.cpp io/qipaddress_p.h
        io/qlockfile.cpp io/qlockfile.h io/qlockfile_p.h
        io/qloggingcategory.cpp io/qloggingcategory.h io/qloggingcategory_p.h
        io/qloggingregistry.cpp io/qloggingregistry_p.h
        io/qnoncontiguousbytedevice.cpp io/qnoncontiguousbytedevice_p.h
        io/qresource.cpp io/qresource.h io/qresource_p.h
        io/qresource_iterator.cpp io/qresource_iterator_p.h
        io/qsavefile.cpp io/qsavefile.h io/qsavefile_p.h
        io/qstandardpaths.cpp io/qstandardpaths.h
        io/qstorageinfo.cpp io/qstorageinfo.h io/qstorageinfo_p.h
        io/qtemporarydir.cpp io/qtemporarydir.h
        io/qtemporaryfile.cpp io/qtemporaryfile.h io/qtemporaryfile_p.h
        io/qurl.cpp io/qurl.h io/qurl_p.h
        io/qurlidna.cpp
        io/qurlquery.cpp io/qurlquery.h
        io/qurlrecode.cpp
        io/qzipreader_p.h io/qzipwriter_p.h io/qzip.cpp
        io/wcharhelpers_win_p.h
        kernel/qabstracteventdispatcher.cpp kernel/qabstracteventdispatcher.h kernel/qabstracteventdispatcher_p.h
        kernel/qabstractnativeeventfilter.cpp kernel/qabstractnativeeventfilter.h
        kernel/qapplicationstatic.h
        kernel/qassociativeiterable.cpp kernel/qassociativeiterable.h
        kernel/qbasictimer.cpp kernel/qbasictimer.h
        kernel/qbindingstorage.h
        kernel/qchronotimer.cpp kernel/qchronotimer.h
        kernel/qcoreapplication.cpp kernel/qcoreapplication.h kernel/qcoreapplication_p.h
        kernel/qcoreapplication_platform.h
        kernel/qcoreevent.cpp kernel/qcoreevent.h kernel/qcoreevent_p.h
        kernel/qdeadlinetimer.cpp kernel/qdeadlinetimer.h
        kernel/qelapsedtimer.cpp kernel/qelapsedtimer.h
        kernel/qeventloop.cpp kernel/qeventloop.h kernel/qeventloop_p.h
        kernel/qfunctions_p.h
        kernel/qiterable.cpp kernel/qiterable.h kernel/qiterable_p.h
        kernel/qmath.cpp kernel/qmath.h
        kernel/qmetacontainer.cpp kernel/qmetacontainer.h
        kernel/qmetaobject.cpp kernel/qmetaobject.h kernel/qmetaobject_p.h
        kernel/qmetaobject_moc_p.h
        kernel/qmetaobjectbuilder.cpp kernel/qmetaobjectbuilder_p.h
        kernel/qmetatype.cpp kernel/qmetatype.h kernel/qmetatype_p.h
        kernel/qmimedata.cpp kernel/qmimedata.h
        kernel/qtmetamacros.h kernel/qtmocconstants.h kernel/qtmochelpers.h
        kernel/qobject.cpp kernel/qobject.h kernel/qobject_p.h kernel/qobject_p_p.h
        kernel/qobject_impl.h
        kernel/qobjectcleanuphandler.cpp kernel/qobjectcleanuphandler.h
        kernel/qobjectdefs.h
        kernel/qobjectdefs_impl.h
        kernel/qpointer.h
        kernel/qproperty.cpp kernel/qproperty.h kernel/qproperty_p.h
        kernel/qpropertyprivate.h
        kernel/qsequentialiterable.cpp kernel/qsequentialiterable.h
        kernel/qsignalmapper.cpp kernel/qsignalmapper.h
        kernel/qsingleshottimer.cpp kernel/qsingleshottimer_p.h
        kernel/qsocketnotifier.cpp kernel/qsocketnotifier.h
        kernel/qsystemerror.cpp kernel/qsystemerror_p.h
        kernel/qtestsupport_core.cpp kernel/qtestsupport_core.h
        kernel/qtimer.cpp kernel/qtimer.h kernel/qtimer_p.h
        kernel/qtranslator.cpp kernel/qtranslator.h kernel/qtranslator_p.h
        kernel/qvariant.cpp kernel/qvariant.h kernel/qvariant_p.h
        kernel/qvariantmap.h kernel/qvarianthash.h kernel/qvariantlist.h
        plugin/qfactoryinterface.cpp plugin/qfactoryinterface.h
        plugin/qfactoryloader.cpp plugin/qfactoryloader_p.h
        plugin/qplugin.h plugin/qplugin_p.h
        plugin/qpluginloader.cpp plugin/qpluginloader.h
        plugin/quuid.cpp plugin/quuid.h plugin/quuid_p.h
        serialization/qcborarray.h
        serialization/qcborcommon.cpp serialization/qcborcommon.h serialization/qcborcommon_p.h
        serialization/qcbordiagnostic.cpp
        serialization/qcbormap.h
        serialization/qcborstream.h
        serialization/qcborvalue.cpp serialization/qcborvalue.h serialization/qcborvalue_p.h
        serialization/qdatastream.cpp serialization/qdatastream.h
        serialization/qjson_p.h
        serialization/qjsonarray.cpp serialization/qjsonarray.h
        serialization/qjsoncbor.cpp
        serialization/qjsondocument.cpp serialization/qjsondocument.h
        serialization/qjsonobject.cpp serialization/qjsonobject.h
        serialization/qjsonparseerror.h
        serialization/qjsonparser.cpp serialization/qjsonparser_p.h
        serialization/qjsonvalue.cpp serialization/qjsonvalue.h
        serialization/qjsonwriter.cpp serialization/qjsonwriter_p.h
        serialization/qtextstream.cpp serialization/qtextstream.h serialization/qtextstream_p.h
        serialization/qxmlutils.cpp serialization/qxmlutils_p.h
        text/qanystringview.cpp text/qanystringview.h
        text/qbytearray.cpp text/qbytearray.h
        text/qbytearrayalgorithms.h
        text/qbytearraylist.cpp text/qbytearraylist.h
        text/qbytearraymatcher.cpp text/qbytearraymatcher.h
        text/qbytearrayview.h
        text/qbytedata_p.h
        text/qchar.h
        text/qcollator.cpp text/qcollator.h text/qcollator_p.h
        text/qdoublescanprint_p.h
        text/qlatin1stringmatcher.cpp text/qlatin1stringmatcher.h
        text/qlatin1stringview.h
        text/qlocale.cpp text/qlocale.h text/qlocale_p.h
        text/qlocale_tools.cpp text/qlocale_tools_p.h
        text/qstaticlatin1stringmatcher.h
        text/qstring.cpp text/qstring.h
        text/qstringalgorithms.h text/qstringalgorithms_p.h
        text/qstringbuilder.cpp text/qstringbuilder.h
        text/qstringconverter_base.h
        text/qstringconverter.cpp text/qstringconverter.h text/qstringconverter_p.h
        text/qstringfwd.h
        text/qstringiterator_p.h
        text/qstringlist.cpp text/qstringlist.h
        text/qstringliteral.h
        text/qstringmatcher.h
        text/qstringtokenizer.cpp text/qstringtokenizer.h
        text/qstringview.cpp text/qstringview.h
        text/qtextboundaryfinder.cpp text/qtextboundaryfinder.h
        text/qtformat_impl.h
        text/qunicodetables_p.h
        text/qunicodetools.cpp text/qunicodetools_p.h
        text/qutf8stringview.h
        text/qvsnprintf.cpp
        thread/qatomic.h
        thread/qatomic_cxx11.h
        thread/qbasicatomic.h
        thread/qgenericatomic.h
        thread/qlocking_p.h
        thread/qmutex.h
        thread/qorderedmutexlocker_p.h
        thread/qreadwritelock.h
        thread/qrunnable.cpp thread/qrunnable.h
        thread/qthread.cpp thread/qthread.h thread/qthread_p.h
        thread/qthreadstorage.h
        thread/qthreadpool.cpp thread/qthreadpool_p.h thread/qthreadpool.h
        thread/qsemaphore.cpp thread/qsemaphore.h
        thread/qtsan_impl.h
        thread/qwaitcondition.h thread/qwaitcondition_p.h
        thread/qyieldcpu.h
        time/qcalendar.cpp time/qcalendar.h
        time/qcalendarbackend_p.h
        time/qcalendarmath_p.h
        time/qdatetime.cpp time/qdatetime.h time/qdatetime_p.h
        time/qgregoriancalendar.cpp time/qgregoriancalendar_p.h
        time/qjuliancalendar.cpp time/qjuliancalendar_p.h
        time/qlocaltime.cpp time/qlocaltime_p.h
        time/qmilankoviccalendar.cpp time/qmilankoviccalendar_p.h
        time/qromancalendar.cpp time/qromancalendar_p.h
        time/qtimezone.cpp time/qtimezone.h
        tools/qalgorithms.h
        tools/qarraydata.cpp tools/qarraydata.h
        tools/qarraydataops.h
        tools/qarraydatapointer.h
        tools/qatomicscopedvaluerollback.h
        tools/qbitarray.cpp tools/qbitarray.h
        tools/qcache.h
        tools/qcontainerfwd.h
        tools/qcontainertools_impl.h
        tools/qcontiguouscache.cpp tools/qcontiguouscache.h
        tools/qcryptographichash.cpp tools/qcryptographichash.h
        tools/qduplicatetracker_p.h
        tools/qflatmap_p.h
        tools/qfreelist.cpp tools/qfreelist_p.h
        tools/qfunctionaltools_impl.cpp tools/qfunctionaltools_impl.h
        tools/qhashfunctions.h
        tools/qiterator.h
        tools/qline.cpp tools/qline.h
        tools/qlist.h
        tools/qmakearray_p.h
        tools/qmap.h
        tools/qmargins.cpp tools/qmargins.h
        tools/qmessageauthenticationcode.h
        tools/qminimalflatset_p.h
        tools/qoffsetstringarray_p.h
        tools/qpair.h
        tools/qpoint.cpp tools/qpoint.h
        tools/qqueue.h
        tools/qrect.cpp tools/qrect.h
        tools/qrefcount.cpp tools/qrefcount.h
        tools/qringbuffer.cpp tools/qringbuffer_p.h
        tools/qscopedpointer.h
        tools/qscopedvaluerollback.h
        tools/qscopeguard.h
        tools/qset.h
        tools/qshareddata.cpp tools/qshareddata.h
        tools/qshareddata_impl.h
        tools/qsharedpointer.cpp tools/qsharedpointer.h
        tools/qsharedpointer_impl.h
        tools/qsize.cpp tools/qsize.h
        tools/qsmallbytearray_p.h
        tools/qspan.h
        tools/qstack.h
        tools/qtaggedpointer.h
        tools/qtools_p.h
        tools/qtyperevision.cpp tools/qtyperevision.h
        tools/quniquehandle_p.h
        tools/quniquehandle_types.cpp tools/quniquehandle_types_p.h
        tools/qvarlengtharray.h
        tools/qvector.h
        tools/qversionnumber.cpp tools/qversionnumber.h
    NO_UNITY_BUILD_SOURCES
        # MinGW complains about `free-nonheap-object` in ~QSharedDataPointer()
        # despite the fact that appropriate checks are in place to avoid that!
        tools/qshareddata.cpp tools/qshareddata.h
        text/qlocale.cpp text/qlocale.h
        global/qassert.cpp # Windows: inconsistent linkage of qAbort()
        global/qglobal.cpp  # undef qFatal
        global/qlogging.cpp # undef qFatal/qInfo/qDebug
        global/qrandom.cpp # undef Q_ASSERT/_X
        text/qstringconverter.cpp # enum Data
        tools/qcryptographichash.cpp # KeccakNISTInterface/Final
        io/qdebug.cpp # undef qDebug
        io/qresource.cpp # QTBUG-132114
    NO_PCH_SOURCES
        compat/removed_api.cpp
        global/qsimd.cpp
        kernel/qsocketnotifier.cpp # defines BUILDING_QSOCKETNOTIFIER
    DEFINES
        QT_NO_CONTEXTLESS_CONNECT
        QT_NO_FOREACH
        QT_NO_QPAIR
        QT_NO_USING_NAMESPACE
        QT_TYPESAFE_FLAGS
        QT_USE_NODISCARD_FILE_OPEN
    INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_BINARY_DIR}/global"
        "${CMAKE_CURRENT_BINARY_DIR}/kernel" # for moc_qobject.cpp to be found by qobject.cpp
        ../3rdparty/tinycbor/src
    LIBRARIES
        Qt::GlobalConfigPrivate
        WrapZLIB::WrapZLIB
    PRECOMPILED_HEADER
        "global/qt_pch.h"
    PUBLIC_LIBRARIES
        Qt::Platform
    EXTRA_CMAKE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/Qt6CTestMacros.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/Qt6CoreConfigureFileTemplate.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/Qt6CoreResourceInit.in.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Qt6CoreDeploySupport.cmake"
        ${corelib_extra_cmake_files}
    POLICIES
        QTP0002
        QTP0003
    ATTRIBUTION_FILE_DIR_PATHS
        text
        global
        ../3rdparty/siphash
        ../3rdparty/blake2
        ../3rdparty/md4
        ../3rdparty/md5
        ../3rdparty/sha1
        ../3rdparty/sha3
        ../3rdparty/rfc6234
        ../3rdparty/tinycbor
    ELF_LINKER_DYNAMIC_LIST
        QtCore.dynlist.in
)
_qt_internal_setup_deploy_support()

add_dependencies(Core qmodule_pri)

if (NOT QT_NAMESPACE STREQUAL "")
    set(core_namespace_defs "QT_NAMESPACE=${QT_NAMESPACE}")
    if(QT_INLINE_NAMESPACE)
        list(APPEND core_namespace_defs QT_INLINE_NAMESPACE)
    endif()
    target_compile_definitions(Core PUBLIC ${core_namespace_defs})
    set_target_properties(Core PROPERTIES _qt_namespace "${QT_NAMESPACE}")
    set_property(TARGET Core APPEND PROPERTY EXPORT_PROPERTIES _qt_namespace)
endif()

qt_generate_qconfig_cpp(global/qconfig.cpp.in global/qconfig.cpp)

set_target_properties(Core PROPERTIES INTERFACE_QT_COORD_TYPE "${QT_COORD_TYPE}")
set_property(TARGET Core APPEND PROPERTY COMPATIBLE_INTERFACE_STRING QT_COORD_TYPE)

# Handle qtConfig(thread): CONFIG += thread like in qt.prf.
# Aka if the feature is enabled, publicly link against the threading library.
# This also ensures the link flag is in the .prl file.
if(QT_FEATURE_thread)
    target_link_libraries(Platform INTERFACE Threads::Threads)
endif()

# Skip AUTOMOC processing of qobject.cpp and its headers.
# We do this on purpose, because qobject.cpp contains a bunch of Q_GADGET, Q_NAMESPACE, etc
# keywords and AUTOMOC gets confused about wanting to compile a qobject.moc file as well.
# Instead use manual moc.
set_source_files_properties(kernel/qobject.cpp kernel/qobject.h kernel/qobject_p.h kernel/qobject_p_p.h
                            PROPERTIES SKIP_AUTOMOC TRUE)

qt_manual_moc(qobject_moc_files
               OUTPUT_MOC_JSON_FILES core_qobject_metatypes_json_list
               kernel/qobject.h)
# The moc file is included directly by qobject.cpp
set_source_files_properties(${qobject_moc_files} PROPERTIES HEADER_FILE_ONLY ON)

set(core_metatype_args MANUAL_MOC_JSON_FILES ${core_qobject_metatypes_json_list})

if(QT_WILL_INSTALL)
    set(metatypes_install_dir ${INSTALL_ARCHDATADIR}/metatypes)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.31")
        cmake_path(SET metatypes_install_dir NORMALIZE "${metatypes_install_dir}")
    endif()
    list(APPEND core_metatype_args
        __QT_INTERNAL_INSTALL __QT_INTERNAL_INSTALL_DIR "${metatypes_install_dir}")
endif()

# Use qt6_extract_metatypes instead so that we can manually pass the
# additional json files.
qt6_extract_metatypes(Core ${core_metatype_args})

target_sources(Core PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/global/qconfig.h"
    "${CMAKE_CURRENT_BINARY_DIR}/global/qconfig_p.h"
)
set_source_files_properties(
    "${CMAKE_CURRENT_BINARY_DIR}/global/qconfig.h"
    "${CMAKE_CURRENT_BINARY_DIR}/global/qconfig_p.h"
    PROPERTIES GENERATED TRUE
)

# Find ELF interpreter and define a macro for that:
if ((LINUX OR HURD) AND NOT CMAKE_CROSSCOMPILING AND BUILD_SHARED_LIBS)
    if (NOT DEFINED ELF_INTERPRETER)
        execute_process(COMMAND ${CMAKE_COMMAND} -E env LC_ALL=C readelf -l /bin/sh
                        RESULT_VARIABLE readelf_ok
                        OUTPUT_VARIABLE readelf_output
                       )
        if ("${readelf_ok}" STREQUAL "0"
                AND "${readelf_output}" MATCHES "program interpreter: (.*)]")
            set(ELF_INTERPRETER "${CMAKE_MATCH_1}" CACHE INTERNAL "ELF interpreter location")
        else()
            set(ELF_INTERPRETER "" CACHE INTERNAL "ELF interpreter location")
        endif()
    endif()
    if (ELF_INTERPRETER)
        target_link_options(Core PRIVATE "-Wl,-e,qt_core_boilerplate")
        target_compile_definitions(Core PRIVATE ELF_INTERPRETER="${ELF_INTERPRETER}")
    endif()
endif()

qt_internal_add_simd_part(Core SIMD mips_dsp
    SOURCES
        ../gui/painting/qt_mips_asm_dsp_p.h
        text/qstring_mips_dsp_asm.S
)

if(ANDROID)
    _qt_internal_add_android_executable_finalizer(Core)
    set_property(TARGET Core APPEND PROPERTY QT_ANDROID_BUNDLED_JAR_DEPENDENCIES
        jar/Qt${QtBase_VERSION_MAJOR}Android.jar
    )
    set_property(TARGET Core APPEND PROPERTY QT_ANDROID_LIB_DEPENDENCIES
        ${INSTALL_PLUGINSDIR}/platforms/libplugins_platforms_qtforandroid.so
    )
    set_property(TARGET Core APPEND PROPERTY QT_ANDROID_PERMISSIONS
        android.permission.INTERNET android.permission.WRITE_EXTERNAL_STORAGE
    )
endif()

# Add version tagging source files if the linker has version script support
# or the platform supports it.
set(core_version_tagging_files
    global/qversiontagging.cpp)
qt_internal_extend_target(Core
    CONDITION QT_FEATURE_version_tagging
    SOURCES ${core_version_tagging_files}
)

if((GCC OR CLANG) AND NOT MSVC)
    # qglobal.cpp has #include moc_qnamespace.cpp, so increase the template
    # depth for it due to the size of the Qt namespace
    set_source_files_properties(global/qglobal.cpp PROPERTIES
        COMPILE_OPTIONS "-ftemplate-depth=2048")
endif()

if(GCC)
    # Disable LTO, as the symbols disappear somehow under GCC
    # (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=48200)
    # This is not tested well with newer GCC versions, so we cannot say if
    # issue persists or not. Assume the disabling is safer option.
    set_source_files_properties(${core_version_tagging_files}
        PROPERTIES COMPILE_OPTIONS "-fno-lto")
endif()


qt_internal_extend_target(Core
    CONDITION ( TEST_architecture_arch STREQUAL i386 ) OR
        ( TEST_architecture_arch STREQUAL x86_64 ) OR
        ( CMAKE_OSX_ARCHITECTURES MATCHES "x86_64" ) OR
        ( CMAKE_OSX_ARCHITECTURES MATCHES "i386" ) OR
    SOURCES
        global/qsimd_x86_p.h
)

qt_internal_extend_target(Core CONDITION ANDROID
    DEFINES
        LIBS_SUFFIX="_${ANDROID_ABI}.so"
)

qt_internal_extend_target(Core CONDITION MSVC AND (TEST_architecture_arch STREQUAL "i386")
    LINK_OPTIONS
        "/BASE:0x67000000"
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_xmlstream
    SOURCES
        serialization/qxmlstream.cpp serialization/qxmlstream.h serialization/qxmlstream_p.h
        serialization/qxmlstreamgrammar.cpp serialization/qxmlstreamgrammar_p.h
        serialization/qxmlstreamparser_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_animation
    SOURCES
        animation/qabstractanimation.cpp animation/qabstractanimation.h animation/qabstractanimation_p.h
        animation/qanimationgroup.cpp animation/qanimationgroup.h animation/qanimationgroup_p.h
        animation/qparallelanimationgroup.cpp animation/qparallelanimationgroup.h animation/qparallelanimationgroup_p.h
        animation/qpauseanimation.cpp animation/qpauseanimation.h
        animation/qpropertyanimation.cpp animation/qpropertyanimation.h animation/qpropertyanimation_p.h
        animation/qsequentialanimationgroup.cpp animation/qsequentialanimationgroup.h animation/qsequentialanimationgroup_p.h
        animation/qvariantanimation.cpp animation/qvariantanimation.h animation/qvariantanimation_p.h
)

# This needs to be done before one below adds kernel32 because the symbols we use
# from synchronization also appears in kernel32 in the version of MinGW we use in CI.
# However, when picking the symbols from libkernel32.a it will try to load the symbols
# from the wrong DLL at runtime and crash!
qt_internal_extend_target(Core CONDITION QT_FEATURE_thread AND WIN32
    SOURCES
        thread/qfutex_win_p.h
        thread/qwaitcondition_win.cpp
    LIBRARIES
        synchronization
)

qt_internal_extend_target(Core CONDITION CYGWIN
    SOURCES
        global/qt_windows.h
)

qt_internal_extend_target(Core CONDITION WIN32
    SOURCES
        global/qoperatingsystemversion_win.cpp global/qoperatingsystemversion_win_p.h
        global/qt_windows.h
        io/qfilesystemengine_win.cpp
        io/qfsfileengine_win.cpp
        io/qlockfile_win.cpp
        io/qstandardpaths_win.cpp
        io/qstorageinfo_win.cpp
        io/qwindowspipereader.cpp io/qwindowspipereader_p.h
        io/qwindowspipewriter.cpp io/qwindowspipewriter_p.h
        io/qntdll_p.h
        kernel/qcoreapplication_win.cpp
        kernel/qeventdispatcher_win.cpp kernel/qeventdispatcher_win_p.h
        kernel/qfunctions_win.cpp kernel/qfunctions_win_p.h kernel/qfunctions_winrt_p.h
        ipc/qsharedmemory_win.cpp
        ipc/qsystemsemaphore_win.cpp
        kernel/qwineventnotifier.cpp kernel/qwineventnotifier.h kernel/qwineventnotifier_p.h
        kernel/qwinregistry.cpp kernel/qwinregistry_p.h
        plugin/qsystemlibrary.cpp plugin/qsystemlibrary_p.h
        text/qlocale_win.cpp
        thread/qthread_win.cpp
        platform/windows/qcomobject_p.h
        platform/windows/qcomptr_p.h
        platform/windows/qbstr_p.h
        platform/windows/qcomvariant_p.h
    LIBRARIES
        advapi32
        authz
        kernel32
        netapi32
        ntdll
        ole32
        runtimeobject
        shell32
        user32
        uuid
        version
        winmm
        ws2_32
    PUBLIC_LIBRARIES
        mpr
        userenv
)

qt_internal_extend_target(Core CONDITION WIN32
    NO_UNITY_BUILD_SOURCES
        global/qsimd.cpp # Q_DECL_INIT_PRIORITY
        serialization/qcborvalue.cpp # various windows.h clashes
        serialization/qjsoncbor.cpp
        serialization/qjsonvalue.cpp
        serialization/qxmlstream.cpp
        text/qbytearray.cpp
        text/qlatin1stringmatcher.cpp
        text/qunicodetools.cpp
        tools/qhash.cpp # Q_DECL_INIT_PRIORITY
        kernel/qwinregistry.cpp # ntstatus.h clashes
)

qt_internal_extend_target(Core CONDITION GCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13"
    NO_UNITY_BUILD_SOURCES
        io/qresource.cpp io/qresource.h io/qresource_p.h # -Werror=subobject-linkage
)

if(NOT WIN32)
    ### Qt7: remove
    # Make qwineventnotifier.h available on non-Windows platforms too for code bases that include
    # it unconditionally.
    qt_internal_extend_target(Core SOURCES kernel/qwineventnotifier.h)
    set_source_files_properties(kernel/qwineventnotifier.h PROPERTIES SKIP_AUTOMOC ON)
endif()

qt_internal_extend_target(Core CONDITION WASM
    SOURCES
        kernel/qcore_wasm.cpp)

qt_internal_extend_target(Core CONDITION APPLE
    SOURCES
        global/qoperatingsystemversion_darwin.mm
        io/qfilesystemengine_mac.mm
        io/qstandardpaths_mac.mm
        io/qstorageinfo_mac.cpp
        kernel/qcfsocketnotifier.cpp kernel/qcfsocketnotifier_p.h
        kernel/qcore_foundation.mm
        kernel/qcore_mac.mm kernel/qcore_mac_p.h
        kernel/qcoreapplication_mac.cpp
        kernel/qeventdispatcher_cf.mm kernel/qeventdispatcher_cf_p.h
    LIBRARIES
        ${FWCoreFoundation}
        ${FWFoundation}
    PUBLIC_LIBRARIES
        ${FWIOKit}
    DEFINES
        _DARWIN_C_SOURCE    # This resolves two issues,
                            #   - Provide DT_* macros to qfilesystemengine_unix.cpp
                            #   - Enables SOCK_MAXADDRLEN in case its missing during the unity build
    NO_UNITY_BUILD_SOURCES
        kernel/qcore_mac.mm # See below
        kernel/qsystemerror.cpp
        # This makes sure that the tst_qmakelib passes. For some reason,
        # QtCore ends up returning a corrupted error message in
        # write_file(): fail
    NO_PCH_SOURCES
        kernel/qcore_mac.mm # See below
    ATTRIBUTION_FILE_DIR_PATHS
        kernel
)

if(APPLE)
    # For QObjCWeakPointer
    set_source_files_properties(kernel/qcore_mac.mm PROPERTIES COMPILE_FLAGS -fobjc-weak)
endif()

qt_internal_extend_target(Core CONDITION MACOS
    LIBRARIES
        ${FWAppKit}
        ${FWApplicationServices}
        ${FWCoreServices}
        ${FWSecurity}
    PUBLIC_LIBRARIES
        ${FWDiskArbitration}
)

qt_internal_extend_target(Core CONDITION INTEGRITY
    LIBRARIES
        ivfs
        net
        posix
        shm_client
        socket
    COMPILE_OPTIONS
        --pending_instantiations=128
)

# Workaround for QTBUG-101411
# Remove if QCC (gcc version 8.3.0) for QNX 7.1.0 is no longer supported
qt_internal_extend_target(Core CONDITION QCC AND (CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "8.3.0")
    PUBLIC_COMPILE_OPTIONS $<$<COMPILE_LANGUAGE:CXX>:-Wno-invalid-offsetof>
)

qt_internal_extend_target(Core CONDITION QNX AND QT_FEATURE_fsnotify
    LIBRARIES
        fsnotify
)

qt_internal_extend_target(Core CONDITION LINUX AND QT_BUILD_SHARED_LIBS
    SOURCES
        global/minimum-linux_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_slog2
    LIBRARIES
        Slog2::Slog2
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_journald
    LIBRARIES
        PkgConfig::Libsystemd
)

qt_internal_extend_target(Core CONDITION UNIX
    SOURCES
        io/qfilesystemengine_unix.cpp
        io/qfilesystemiterator_unix.cpp
        io/qfsfileengine_unix.cpp
        io/qlockfile_unix.cpp
        kernel/qcore_unix.cpp kernel/qcore_unix_p.h
        kernel/qpoll_p.h
        kernel/qtimerinfo_unix.cpp kernel/qtimerinfo_unix_p.h
        thread/qthread_unix.cpp
)
if(APPLE)
    set_source_files_properties(io/qfilesystemengine_unix.cpp PROPERTIES LANGUAGE OBJCXX)
    qt_internal_extend_target(Core CONDITION
        PUBLIC_LIBRARIES ${FWUniformTypeIdentifiers}
    )
endif()

qt_internal_extend_target(Core CONDITION UNIX AND NOT WASM
    SOURCES
        kernel/qeventdispatcher_unix.cpp kernel/qeventdispatcher_unix_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_thread
    SOURCES
        thread/qatomic.cpp
        thread/qfutex_p.h
        thread/qmutex.cpp thread/qmutex_p.h
        thread/qreadwritelock.cpp thread/qreadwritelock_p.h
        thread/qthreadstorage.cpp thread/qthreadstorage_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_thread AND UNIX
    SOURCES
        thread/qwaitcondition_unix.cpp
)

qt_internal_extend_target(Core CONDITION APPLE AND QT_FEATURE_thread
    SOURCES
        thread/qfutex_mac_p.h
        thread/qmutex_mac.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_thread AND UNIX AND NOT APPLE AND NOT LINUX
    SOURCES
        thread/qmutex_unix.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_thread AND FREEBSD
    SOURCES
        thread/qfutex_freebsd_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_thread AND LINUX
    SOURCES
        thread/qfutex_linux_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_future
    SOURCES
        thread/qexception.cpp thread/qexception.h
        thread/qfuture.h
        thread/qfuture_impl.h
        thread/qfutureinterface.cpp thread/qfutureinterface.h thread/qfutureinterface_p.h
        thread/qfuturesynchronizer.h
        thread/qfuturewatcher.cpp thread/qfuturewatcher.h thread/qfuturewatcher_p.h
        thread/qpromise.h
        thread/qresultstore.cpp thread/qresultstore.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_std_atomic64
    PUBLIC_LIBRARIES
        WrapAtomic::WrapAtomic
)

qt_internal_extend_target(Core CONDITION NOT QT_FEATURE_system_zlib
    LIBRARIES
        Qt::ZlibPrivate
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_commandlineparser
    SOURCES
        tools/qcommandlineoption.cpp tools/qcommandlineoption.h
        tools/qcommandlineparser.cpp tools/qcommandlineparser.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_backtrace
    DEFINES
        BACKTRACE_HEADER="${Backtrace_HEADER}"
    LIBRARIES
        WrapBacktrace::WrapBacktrace
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_system_doubleconversion
    LIBRARIES
        WrapSystemDoubleConversion::WrapSystemDoubleConversion
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_doubleconversion AND NOT QT_FEATURE_system_doubleconversion
    SOURCES
        ../3rdparty/double-conversion/double-conversion/bignum.cc
        ../3rdparty/double-conversion/double-conversion/bignum-dtoa.cc
        ../3rdparty/double-conversion/double-conversion/bignum-dtoa.h
        ../3rdparty/double-conversion/double-conversion/bignum.h
        ../3rdparty/double-conversion/double-conversion/cached-powers.cc
        ../3rdparty/double-conversion/double-conversion/cached-powers.h
        ../3rdparty/double-conversion/double-conversion/diy-fp.h
        ../3rdparty/double-conversion/double-conversion/double-conversion.h
        ../3rdparty/double-conversion/double-conversion/double-to-string.cc
        ../3rdparty/double-conversion/double-conversion/double-to-string.h
        ../3rdparty/double-conversion/double-conversion/fast-dtoa.cc
        ../3rdparty/double-conversion/double-conversion/fast-dtoa.h
        ../3rdparty/double-conversion/double-conversion/fixed-dtoa.cc
        ../3rdparty/double-conversion/double-conversion/fixed-dtoa.h
        ../3rdparty/double-conversion/double-conversion/ieee.h
        ../3rdparty/double-conversion/double-conversion/string-to-double.cc
        ../3rdparty/double-conversion/double-conversion/string-to-double.h
        ../3rdparty/double-conversion/double-conversion/strtod.cc
        ../3rdparty/double-conversion/double-conversion/strtod.h
        ../3rdparty/double-conversion/double-conversion/utils.h
    INCLUDE_DIRECTORIES
        ../3rdparty/double-conversion/double-conversion
        ../3rdparty/double-conversion
    ATTRIBUTION_FILE_DIR_PATHS
        ../3rdparty/double-conversion
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_system_libb2
    LIBRARIES
        Libb2::Libb2
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_easingcurve
    SOURCES
        tools/qeasingcurve.cpp tools/qeasingcurve.h
        tools/qtimeline.cpp tools/qtimeline.h
    ATTRIBUTION_FILE_DIR_PATHS
        ../3rdparty/easing
)

qt_internal_extend_target(Core CONDITION UNIX AND NOT HAIKU AND NOT INTEGRITY AND NOT VXWORKS AND NOT WASM AND NOT MACOS
    LIBRARIES
        m
)

qt_internal_extend_target(Core CONDITION APPLE
    SOURCES
        text/qlocale_mac.mm
)

qt_internal_extend_target(Core CONDITION UNIX AND NOT APPLE AND NOT WASM
    SOURCES
        text/qlocale_unix.cpp
)

# Switch this to just 'icu' once we can expect Windows 10 version 1903
qt_internal_extend_target(Core CONDITION QT_FEATURE_winsdkicu
    LIBRARIES
        icuuc icuin
)

qt_internal_extend_target(Core CONDITION WASM
    SOURCES
        text/qlocale_wasm.cpp
)

qt_internal_extend_target(Core CONDITION MSVC AND CLANG
    LIBRARIES
        clang_rt.builtins-x86_64
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_icu
    SOURCES
        text/qcollator_icu.cpp
        text/qlocale_icu.cpp
    LIBRARIES
        ICU::i18n ICU::uc ICU::data
)

qt_internal_extend_target(Core CONDITION WIN32 AND NOT QT_FEATURE_icu
    SOURCES
        text/qcollator_win.cpp
)

qt_internal_extend_target(Core CONDITION MACOS AND NOT QT_FEATURE_icu
    SOURCES
        text/qcollator_macx.cpp
)

qt_internal_extend_target(Core CONDITION UNIX AND NOT MACOS AND NOT QT_FEATURE_icu
    SOURCES
        text/qcollator_posix.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_jemalloc
    PUBLIC_LIBRARIES
        PkgConfig::JeMalloc
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_regularexpression
    SOURCES
        text/qregularexpression.cpp text/qregularexpression.h
    LIBRARIES
        WrapPCRE2::WrapPCRE2
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_openssl_hash
    LIBRARIES
        WrapOpenSSL::WrapOpenSSL
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_hijricalendar
    SOURCES
        time/qhijricalendar.cpp time/qhijricalendar_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_islamiccivilcalendar
    SOURCES
        time/qislamiccivilcalendar.cpp time/qislamiccivilcalendar_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_jalalicalendar
    SOURCES
        time/qjalalicalendar.cpp time/qjalalicalendar_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_timezone
    SOURCES
        time/qtimezoneprivate.cpp time/qtimezoneprivate_p.h
)

qt_internal_extend_target(Core
    CONDITION
        QT_FEATURE_timezone_tzdb
    SOURCES
        time/qtimezoneprivate_chrono.cpp
)

qt_internal_extend_target(Core
    CONDITION
        QT_FEATURE_timezone AND APPLE AND NOT QT_FEATURE_timezone_tzdb
    SOURCES
        time/qtimezoneprivate_mac.mm
)

qt_internal_extend_target(Core
    CONDITION
        QT_FEATURE_timezone AND ANDROID
        AND NOT APPLE AND NOT QT_FEATURE_timezone_tzdb
    SOURCES
        time/qtimezoneprivate_android.cpp
)

qt_internal_extend_target(Core
    CONDITION
        QT_FEATURE_timezone AND UNIX
        AND NOT ANDROID AND NOT APPLE AND NOT QT_FEATURE_timezone_tzdb
    SOURCES
        time/qtimezoneprivate_tz.cpp
)

qt_internal_extend_target(Core
    CONDITION
        QT_FEATURE_icu AND QT_FEATURE_timezone
        AND NOT UNIX AND NOT QT_FEATURE_timezone_tzdb
    SOURCES
        time/qtimezoneprivate_icu.cpp
)

# Even MS says we should prefer ICU over its APIs for TZ data:
qt_internal_extend_target(Core
    CONDITION
        QT_FEATURE_timezone AND WIN32 AND NOT QT_FEATURE_icu
        AND NOT QT_FEATURE_timezone_tzdb
    SOURCES
        time/qtimezoneprivate_win.cpp
)

qt_internal_extend_target(Core
    CONDITION
        QT_FEATURE_timezone_locale
    SOURCES
        time/qtimezonelocale.cpp time/qtimezonelocale_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_datetimeparser
    SOURCES
        time/qdatetimeparser.cpp time/qdatetimeparser_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_zstd
    LIBRARIES
        WrapZSTD::WrapZSTD
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_filesystemwatcher
    SOURCES
        io/qfilesystemwatcher.cpp io/qfilesystemwatcher.h io/qfilesystemwatcher_p.h
        io/qfilesystemwatcher_polling.cpp io/qfilesystemwatcher_polling_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_filesystemwatcher AND WIN32
    SOURCES
        io/qfilesystemwatcher_win.cpp io/qfilesystemwatcher_win_p.h
)

qt_internal_extend_target(Core CONDITION MACOS AND QT_FEATURE_filesystemwatcher
    SOURCES
        io/qfilesystemwatcher_fsevents.mm io/qfilesystemwatcher_fsevents_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_filesystemwatcher AND QT_FEATURE_inotify AND UNIX AND NOT MACOS
    SOURCES
        io/qfilesystemwatcher_inotify.cpp io/qfilesystemwatcher_inotify_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_filesystemwatcher AND UNIX AND NOT MACOS AND NOT QT_FEATURE_inotify AND (APPLE OR FREEBSD OR NETBSD OR OPENBSD)
    SOURCES
        io/qfilesystemwatcher_kqueue.cpp io/qfilesystemwatcher_kqueue_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_processenvironment
    SOURCES
        io/qprocess.cpp io/qprocess.h io/qprocess_p.h
)

if (NOT QT_FEATURE_process)
    set_source_files_properties(io/qprocess.cpp io/qprocess.h PROPERTIES SKIP_AUTOMOC TRUE)
endif()

qt_internal_extend_target(Core CONDITION QT_FEATURE_processenvironment AND WIN32
    SOURCES
        io/qprocess_win.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_processenvironment AND UNIX
    SOURCES
        io/qprocess_unix.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_settings
    SOURCES
        io/qsettings.cpp io/qsettings.h io/qsettings_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_settings AND WIN32
    SOURCES
        io/qsettings_win.cpp
)

qt_internal_extend_target(Core CONDITION APPLE AND QT_FEATURE_settings
    SOURCES
        io/qsettings_mac.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_settings AND WASM
    SOURCES
        io/qsettings_wasm.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_filesystemiterator AND WIN32
    SOURCES
        io/qfilesystemiterator_win.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_process AND UNIX
    SOURCES
        ../3rdparty/forkfd/forkfd.h
        io/forkfd_qt.c
    INCLUDE_DIRECTORIES
        ../3rdparty/forkfd
    ATTRIBUTION_FILE_DIR_PATHS
        ../3rdparty/forkfd
)

qt_internal_extend_target(Core CONDITION APPLE AND QT_FEATURE_processenvironment
    SOURCES
        io/qprocess_darwin.mm
)

qt_internal_extend_target(Core CONDITION APPLE AND NOT MACOS
    PUBLIC_LIBRARIES
        ${FWMobileCoreServices}
)

qt_internal_extend_target(Core CONDITION ANDROID
    SOURCES
        io/qstandardpaths_android.cpp
        io/qstorageinfo_linux.cpp io/qstorageinfo_linux_p.h
        kernel/qjniarray.h
        kernel/qjnitypes.h kernel/qjnitypes_impl.h
        kernel/qjnienvironment.cpp kernel/qjnienvironment.h
        kernel/qjniobject.cpp kernel/qjniobject.h
        kernel/qjnihelpers.cpp kernel/qjnihelpers_p.h
        platform/android/qandroidextras_p.h platform/android/qandroidextras.cpp
        platform/android/qandroidnativeinterface.cpp
        platform/android/qandroidtypes_p.h
        platform/android/qandroidtypeconverter_p.h
        platform/android/qandroiditemmodelproxy_p.h platform/android/qandroiditemmodelproxy.cpp
        platform/android/qandroidmodelindexproxy_p.h platform/android/qandroidmodelindexproxy.cpp
    NO_UNITY_BUILD_SOURCES
        platform/android/qandroidextras.cpp
        # qtNativeClassName conflicts with similar symbols in android headers
        # TODO: Resolve conflicts between various variables set as,
        # `org/qtproject/qt/android/QtNative` QtAndroidPrivate might be a good
        # place to put them.
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_cpp_winrt
    SOURCES
        platform/windows/qfactorycacheregistration_p.h
        platform/windows/qfactorycacheregistration.cpp
        platform/windows/qt_winrtbase_p.h
)

qt_internal_extend_target(Core CONDITION HAIKU AND NOT ANDROID
    SOURCES
        io/qstandardpaths_haiku.cpp
        io/qstorageinfo_unix.cpp
    PUBLIC_LIBRARIES
        be
)

qt_internal_extend_target(Core
    CONDITION UNIX AND NOT LINUX AND NOT APPLE AND NOT HAIKU AND NOT ANDROID AND NOT VXWORKS
    SOURCES
        io/qstandardpaths_unix.cpp
        io/qstorageinfo_unix.cpp
)

qt_internal_extend_target(Core CONDITION LINUX AND NOT ANDROID AND NOT VXWORKS
    SOURCES
        io/qstandardpaths_unix.cpp
        io/qstorageinfo_linux.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_itemmodel
    SOURCES
        itemmodels/qabstractitemmodel.cpp itemmodels/qabstractitemmodel.h itemmodels/qabstractitemmodel_p.h
        itemmodels/qitemselectionmodel.cpp itemmodels/qitemselectionmodel.h itemmodels/qitemselectionmodel_p.h
        itemmodels/qrangemodel.h itemmodels/qrangemodel_impl.h itemmodels/qrangemodel.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_proxymodel
    SOURCES
        itemmodels/qabstractproxymodel.cpp itemmodels/qabstractproxymodel.h itemmodels/qabstractproxymodel_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_concatenatetablesproxymodel AND QT_FEATURE_proxymodel
    SOURCES
        itemmodels/qconcatenatetablesproxymodel.cpp itemmodels/qconcatenatetablesproxymodel.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_identityproxymodel AND QT_FEATURE_proxymodel
    SOURCES
        itemmodels/qidentityproxymodel.cpp itemmodels/qidentityproxymodel.h itemmodels/qidentityproxymodel_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_proxymodel AND QT_FEATURE_sortfilterproxymodel
    SOURCES
        itemmodels/qsortfilterproxymodel.cpp itemmodels/qsortfilterproxymodel.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_proxymodel AND QT_FEATURE_transposeproxymodel
    SOURCES
        itemmodels/qtransposeproxymodel.cpp itemmodels/qtransposeproxymodel.h itemmodels/qtransposeproxymodel_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_stringlistmodel
    SOURCES
        itemmodels/qstringlistmodel.cpp itemmodels/qstringlistmodel.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_library
    SOURCES
        plugin/qlibrary.cpp plugin/qlibrary.h plugin/qlibrary_p.h
)
qt_internal_extend_target(Core CONDITION QT_FEATURE_library AND CYGWIN
    SOURCES
        plugin/qcoffpeparser.cpp plugin/qcoffpeparser_p.h
)
qt_internal_extend_target(Core CONDITION QT_FEATURE_library AND WIN32
    SOURCES
        plugin/qcoffpeparser.cpp plugin/qcoffpeparser_p.h
        plugin/qlibrary_win.cpp
)
qt_internal_extend_target(Core CONDITION QT_FEATURE_library AND APPLE
    SOURCES
        plugin/qlibrary_unix.cpp
        plugin/qmachparser.cpp plugin/qmachparser_p.h
)
qt_internal_extend_target(Core CONDITION QT_FEATURE_library AND UNIX AND NOT APPLE
    SOURCES
        plugin/qelfparser_p.cpp plugin/qelfparser_p.h
        plugin/qlibrary_unix.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_dlopen
    LIBRARIES
        ${CMAKE_DL_LIBS}
)

qt_internal_extend_target(Core CONDITION APPLE AND UIKIT
    LIBRARIES
        ${FWUIKit}
)

qt_internal_extend_target(Core CONDITION WATCHOS
    LIBRARIES
        ${FWWatchKit}
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_poll_select AND UNIX
    SOURCES
        kernel/qpoll.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_glib AND UNIX
    SOURCES
        kernel/qeventdispatcher_glib.cpp kernel/qeventdispatcher_glib_p.h
    LIBRARIES
        GLIB2::GLIB2
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_clock_gettime
    LIBRARIES
        WrapRt::WrapRt
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_posix_shm AND UNIX
    SOURCES
        ipc/qsharedmemory_posix.cpp
    LIBRARIES
        WrapRt::WrapRt
)
qt_internal_extend_target(Core CONDITION QT_FEATURE_sysv_shm
    SOURCES
        ipc/qsharedmemory_systemv.cpp
)
qt_internal_extend_target(Core CONDITION QT_FEATURE_posix_sem
    SOURCES
        ipc/qsystemsemaphore_posix.cpp
    LIBRARIES
        WrapRt::WrapRt
)
qt_internal_extend_target(Core CONDITION QT_FEATURE_sysv_sem
    SOURCES
        ipc/qsystemsemaphore_systemv.cpp
)

qt_internal_extend_target(Core CONDITION VXWORKS
    SOURCES
        io/qstandardpaths_unix.cpp
        io/qstorageinfo_stub.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_cborstreamreader
    SOURCES
        serialization/qcborstreamreader.cpp serialization/qcborstreamreader.h
    NO_UNITY_BUILD_SOURCES
        serialization/qcborstreamreader.cpp # some problem with cbor_value_get_type etc
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_cborstreamwriter
    SOURCES
        serialization/qcborstreamwriter.cpp serialization/qcborstreamwriter.h
    NO_UNITY_BUILD_SOURCES
        serialization/qcborstreamwriter.cpp # CBOR macro clashes
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_mimetype
    SOURCES
        mimetypes/qmimedatabase.cpp mimetypes/qmimedatabase.h mimetypes/qmimedatabase_p.h
        mimetypes/qmimeglobpattern.cpp mimetypes/qmimeglobpattern_p.h
        mimetypes/qmimemagicrule.cpp mimetypes/qmimemagicrule_p.h
        mimetypes/qmimemagicrulematcher.cpp mimetypes/qmimemagicrulematcher_p.h
        mimetypes/qmimeprovider.cpp mimetypes/qmimeprovider_p.h
        mimetypes/qmimetype.cpp mimetypes/qmimetype.h mimetypes/qmimetype_p.h
        mimetypes/qmimetypeparser.cpp mimetypes/qmimetypeparser_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_permissions
    SOURCES
        kernel/qpermissions.cpp kernel/qpermissions.h kernel/qpermissions_p.h
)

if(QT_FEATURE_permissions AND APPLE)
    qt_internal_extend_target(Core
        SOURCES
            kernel/qpermissions_darwin.mm
            platform/darwin/qdarwinpermissionplugin.mm
            platform/darwin/qdarwinpermissionplugin_p.h
        PLUGIN_TYPES
            permissions
    )

    foreach(permission Camera Microphone Bluetooth Contacts Calendar Location)
        qt_internal_add_darwin_permission_plugin("${permission}")
    endforeach()

    # Camera
    qt_internal_extend_target(QDarwinCameraPermissionPlugin
        LIBRARIES ${FWAVFoundation}
    )
    set_property(TARGET QDarwinCameraPermissionPlugin PROPERTY
        _qt_darwin_permissison_separate_request TRUE
    )

    # Microphone
    qt_internal_extend_target(QDarwinMicrophonePermissionPlugin
        LIBRARIES ${FWAVFoundation}
    )
    set_property(TARGET QDarwinMicrophonePermissionPlugin PROPERTY
        _qt_darwin_permissison_separate_request TRUE
    )

    # Bluetooth
    qt_internal_extend_target(QDarwinBluetoothPermissionPlugin
        LIBRARIES ${FWCoreBluetooth}
    )
    set_property(TARGET QDarwinBluetoothPermissionPlugin PROPERTY
        _qt_info_plist_usage_descriptions "NSBluetoothAlwaysUsageDescription"
    )

    # Contacts
    qt_internal_extend_target(QDarwinContactsPermissionPlugin
        LIBRARIES ${FWContacts}
    )

    # Calendar
    qt_internal_extend_target(QDarwinCalendarPermissionPlugin
        LIBRARIES ${FWEventKit}
    )
    set_property(TARGET QDarwinCalendarPermissionPlugin PROPERTY
        _qt_info_plist_usage_descriptions "NSCalendarsUsageDescription"
    )

    # Location
    qt_internal_extend_target(QDarwinLocationPermissionPlugin
        LIBRARIES ${FWCoreLocation}
    )
    if(MACOS)
        set_property(TARGET QDarwinLocationPermissionPlugin PROPERTY
            _qt_info_plist_usage_descriptions
                "NSLocationUsageDescription"
        )
    else()
        set_property(TARGET QDarwinLocationPermissionPlugin PROPERTY
            _qt_info_plist_usage_descriptions
                "NSLocationWhenInUseUsageDescription"
                "NSLocationAlwaysAndWhenInUseUsageDescription"
        )
    endif()
endif()

qt_internal_extend_target(Core CONDITION QT_FEATURE_permissions AND ANDROID
    SOURCES
        kernel/qpermissions_android.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_permissions AND WASM
    SOURCES
        kernel/qpermissions_wasm.cpp
)

if(QT_FEATURE_mimetype AND QT_FEATURE_mimetype_database)
    include(${CMAKE_CURRENT_SOURCE_DIR}/mimetypes/mimetypes_resources.cmake)

    if(CMAKE_VERSION VERSION_LESS 3.18 OR QT_AVOID_CMAKE_ARCHIVING_API)
        set(archiving_api "External")
    else()
        set(archiving_api "CMake")
        if(QT_FEATURE_zstd AND NOT QT_CMAKE_ZSTD_SUPPORT)
            message(FATAL_ERROR
                "CMake was not built with zstd support. "
                "Rebuild CMake or set QT_AVOID_CMAKE_ARCHIVING_API=ON.")
        endif()
    endif()

    if(DEFINED INPUT_mimetype_database_compression)
        set(supported_compression_types zstd gzip none)
        if(INPUT_mimetype_database_compression IN_LIST supported_compression_types)
            set(compression_type ${INPUT_mimetype_database_compression})
        else()
            message(FATAL_ERROR "Unknown mime type database compression is set:"
            " ${INPUT_mimetype_database_compression}\nSupported compression types:\n"
            "    ${supported_compression_types}")
        endif()
        if(compression_type STREQUAL "zstd" AND NOT QT_FEATURE_zstd)
            message(FATAL_ERROR
                "zstd compression is selected for mime type database, but the 'zstd'"
                " feature is disabled.")
        endif()
    elseif(QT_FEATURE_zstd)
        set(compression_type "zstd")
    else()
        set(compression_type "gzip")
    endif()

    if(QT_INTERNAL_ENABLE_VERBOSE_MIME_DATABASE_COMPRESSION)
        set(extra_mime_db_compressor_flags "--log-level=STATUS")
    else()
        set(extra_mime_db_compressor_flags "--log-level=NOTICE")
    endif()

    # Generate qmimeprovider_database.cpp
    set(qmimeprovider_db_output "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmimeprovider_database.cpp")
    add_custom_command(OUTPUT "${qmimeprovider_db_output}"
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${corelib_mimetypes_resource_file}
            -DOUTPUT_FILE=${qmimeprovider_db_output}
            -DARCHIVING_API=${archiving_api}
            -DCOMPRESSION_TYPE=${compression_type}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/QtCompressMimeDatabase.cmake"
            ${extra_mime_db_compressor_flags}
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/QtCompressMimeDatabase.cmake"
            "${corelib_mimetypes_resource_file}"
        VERBATIM
    )

    qt_internal_extend_target(Core
        SOURCES ${qmimeprovider_db_output}
        INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/.rcc"
        ATTRIBUTION_FILE_DIR_PATHS
            mimetypes/3rdparty
    )
    set_source_files_properties(${qmimeprovider_db_output} PROPERTIES
        GENERATED TRUE
        HEADER_FILE_ONLY TRUE
    )
endif()

qt_internal_extend_target(Core CONDITION WASM
    SOURCES
        platform/wasm/qwasmglobal.cpp platform/wasm/qwasmglobal_p.h
        platform/wasm/qstdweb.cpp platform/wasm/qstdweb_p.h
        platform/wasm/qwasmsocket.cpp platform/wasm/qwasmsocket_p.h
        platform/wasm/qwasmsuspendresumecontrol.cpp platform/wasm/qwasmsuspendresumecontrol_p.h
        kernel/qeventdispatcher_wasm.cpp kernel/qeventdispatcher_wasm_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_ctf
    SOURCES
        tracing/qctf_p.h tracing/qctf.cpp
    PLUGIN_TYPES
        tracing
)

# These files are included by qmutex.cpp
set_source_files_properties(
    thread/qmutex_mac.cpp
    thread/qmutex_unix.cpp
    PROPERTIES HEADER_FILE_ONLY ON)

# Remove QT_NO_CAST_TO_ASCII to ensure that the symbols are included in the library.
if(WIN32)
    get_target_property(defines Core COMPILE_DEFINITIONS)
    list(REMOVE_ITEM defines QT_NO_CAST_TO_ASCII)
    set_target_properties(Core PROPERTIES COMPILE_DEFINITIONS "${defines}")
endif()

qt_internal_apply_gc_binaries_conditional(Core PUBLIC)

# Add entry-point on platforms that need it. A project can opt-out of using the
# entrypoint by setting the qt_no_entrypoint property to TRUE on a target.
if(WIN32 OR UIKIT)
    # find_package(Qt6Core) should call find_package(Qt6EntryPointPrivate) so that we can
    # link against EntryPointPrivate. Normally this is handled automatically for deps, but
    # it doesn't work for EntryPointPrivate since its linker line contains the generator expression.
    qt_internal_register_target_dependencies(Core PUBLIC Qt6::EntryPointPrivate)

    set(entrypoint_conditions "$<NOT:$<BOOL:$<TARGET_PROPERTY:qt_no_entrypoint>>>")
    list(APPEND entrypoint_conditions "$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>")

    if(WIN32)
        list(APPEND entrypoint_conditions "$<BOOL:$<TARGET_PROPERTY:WIN32_EXECUTABLE>>")
    endif()

    list(JOIN entrypoint_conditions "," entrypoint_conditions)
    set(entrypoint_conditions "$<AND:${entrypoint_conditions}>")

    target_link_libraries(Core INTERFACE
        "$<${entrypoint_conditions}:${QT_CMAKE_EXPORT_NAMESPACE}::EntryPointPrivate>"
    )
endif()

# Record darwin minimum deployment target.
if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    set_property(TARGET Core PROPERTY
                 QT_DARWIN_MIN_DEPLOYMENT_TARGET "${CMAKE_OSX_DEPLOYMENT_TARGET}")
    set_property(TARGET Core APPEND PROPERTY
                 EXPORT_PROPERTIES "QT_DARWIN_MIN_DEPLOYMENT_TARGET")
endif()

qt_internal_generate_tracepoints(Core core
    SOURCES
        kernel/qcoreapplication.cpp
        kernel/qcoreevent.cpp
        kernel/qobject.cpp
        plugin/qfactoryloader.cpp
        plugin/qlibrary.cpp
        global/qlogging.cpp
)
qt_internal_add_docs(Core
    doc/qtcore.qdocconf
)

qt_internal_add_optimize_full_flags()

# Copy / install an lldb python script into the QtCore.framework.dSYM bundle which searches
# for the latest installed Qt Creator and loads its lldbbridge.py script.
# When debugging a Qt app, lldb will prompt the developer to explicitly import the shim script.
# It will then enable Qt C++ type pretty printers when using command-line lldb or Xcode
# (e.g. show contents of QString).
if(APPLE AND QT_FEATURE_framework AND QT_FEATURE_separate_debug_info)
    qt_internal_module_info(dsym_module_name "Core")

    set(dsym_dir "${dsym_module_name}.framework.dSYM")
    set(script_name "${dsym_module_name}.py")
    set(dsym_script_dir_suffix "${INSTALL_LIBDIR}/${dsym_dir}/Contents/Resources/Python")
    set(dsym_script_build_path "${QT_BUILD_DIR}/${dsym_script_dir_suffix}/${script_name}")

    qt_path_join(dsym_script_install_dir
                 ${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX} ${dsym_script_dir_suffix})

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/debug_script.py"
        "${dsym_script_build_path}"
        @ONLY
    )
    qt_install(FILES
        "${dsym_script_build_path}"
        DESTINATION "${dsym_script_install_dir}"
    )
endif()

if(IOS)
    qt_internal_set_apple_privacy_manifest(Core
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/ios/PrivacyInfo.xcprivacy")
endif()

set(elf_linker_script_contents "")
if (QT_NAMESPACE STREQUAL "")
    set(tag_symbol "qt_version_tag")
else()
    set(tag_symbol "qt_version_tag_${QT_NAMESPACE}")
endif()
foreach(minor_version RANGE ${PROJECT_VERSION_MINOR})
    set(previous "${current}")
    set(current "Qt_${PROJECT_VERSION_MAJOR}.${minor_version}")
    if (minor_version EQUAL ${PROJECT_VERSION_MINOR})
        string(APPEND elf_linker_script_contents "${current} { ${tag_symbol}; } ${previous};\n")
    else()
        string(APPEND elf_linker_script_contents "${current} {} ${previous};\n")
    endif()
endforeach()
qt_internal_extend_target(Core
    EXTRA_ELF_LINKER_SCRIPT_CONTENT "${elf_linker_script_contents}"

    # Workaround for QTBUG-117514:
    # Function called by inline methods taking a pointer to a private class as a parameter
    EXTRA_ELF_LINKER_SCRIPT_EXPORTS
        # QFutureInterfaceBase::setContinuation(std::function<void (QFutureInterfaceBase const&)>, QFutureInterfaceBasePrivate*)
        "_ZN*20QFutureInterfaceBase15setContinuationE*27QFutureInterfaceBasePrivate*"
        # QReadWriteLock::destroyRecursive(QReadWriteLockPrivate*)
        "_ZN*14QReadWriteLock16destroyRecursiveEP*21QReadWriteLockPrivate*"
)

function(qt_internal_library_deprecation_level)
    # QT_DISABLE_DEPRECATED_UP_TO controls which version we use as a cut-off
    # compiling in to the library. E.g. if it is set to QT_VERSION then no
    # code which was deprecated before QT_VERSION will be compiled in.
    set(digit "[0-9a-fA-F]")
    string(REPEAT "${digit}" 6 six_digits)
    if(NOT DEFINED QT_DISABLE_DEPRECATED_UP_TO)
        if(WIN32)
            # On Windows, due to the way DLLs work, we need to export all functions,
            # including the inlines
            set(QT_DISABLE_DEPRECATED_UP_TO "0x040800")
        else()
            # On other platforms, Qt's own compilation does need to compile the Qt 5.0 API
            set(QT_DISABLE_DEPRECATED_UP_TO "0x050000")
        endif()
    elseif(NOT QT_DISABLE_DEPRECATED_UP_TO MATCHES "^0x${six_digits}$")
        message(FATAL_ERROR "Invalid format of the QT_DISABLE_DEPRECATED_UP_TO macro:"
            " ${QT_DISABLE_DEPRECATED_UP_TO}. The expected format is the hexadecimal number,"
            " e.g. 0x060102")
    endif()
    # QT_WARN_DEPRECATED_UP_TO controls the upper-bound of deprecation
    # warnings that are emitted. E.g. if it is set to 0x060500 then all use of
    # things deprecated in or before 6.5.0 will be warned against.
    set(QT_WARN_DEPRECATED_UP_TO 0x070000)

    set(output_header "${CMAKE_CURRENT_BINARY_DIR}/global/qtdeprecationdefinitions.h")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/global/qtdeprecationdefinitions.h.in"
        "${output_header}"
        @ONLY
        NEWLINE_STYLE UNIX
    )

    target_sources(Core PRIVATE "${output_header}")
    set_source_files_properties("${output_header}" PROPERTIES GENERATED TRUE)
endfunction()
qt_internal_library_deprecation_level()

if(QT_FEATURE_doc_snippets)
    add_subdirectory(doc/snippets)
endif()

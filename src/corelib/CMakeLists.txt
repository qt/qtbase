# Generated from corelib.pro.

# special case begin
qt_find_package(Threads PROVIDED_TARGETS Threads::Threads)
qt_find_package(WrapPCRE2 PROVIDED_TARGETS WrapPCRE2::WrapPCRE2)


# compute the reverse relative path from QtCoreConfigExtras to the install prefix
# this is used in QtCoreConfigExtras to make its install paths relocatable
if(QT_WILL_INSTALL)
    get_filename_component(_clean_prefix
        "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${QT_CONFIG_INSTALL_DIR}" ABSOLUTE)
else()
    get_filename_component(_clean_prefix "${QT_CONFIG_BUILD_DIR}" ABSOLUTE)
endif()
file(RELATIVE_PATH QT_INVERSE_CONFIG_INSTALL_DIR
    "${_clean_prefix}" "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")

if(ANDROID)
    set(corelib_extra_cmake_files
        "${CMAKE_CURRENT_SOURCE_DIR}/${QT_CMAKE_EXPORT_NAMESPACE}AndroidMacros.cmake")
endif()
if(WASM)
    set(corelib_extra_cmake_files
        "${CMAKE_CURRENT_SOURCE_DIR}/${QT_CMAKE_EXPORT_NAMESPACE}WasmMacros.cmake")
endif()
# special case end

#####################################################################
## Core Module:
#####################################################################

qt_internal_add_module(Core
    QMAKE_MODULE_CONFIG moc resources
    NO_GENERATE_METATYPES # metatypes are extracted manually below
    EXCEPTIONS
    SOURCES
        global/archdetect.cpp
        global/qcompare.h
        global/qcompilerdetection.h
        global/qcontainerinfo.h
        global/qendian.cpp global/qendian.h global/qendian_p.h
        global/qflags.h
        global/qfloat16.cpp global/qfloat16.h
        global/qglobal.cpp global/qglobal.h
        global/qglobalstatic.h
        global/qhooks.cpp global/qhooks_p.h
        global/qlibraryinfo.cpp global/qlibraryinfo.h global/qlibraryinfo_p.h
        global/qlogging.cpp global/qlogging.h
        global/qmalloc.cpp
        global/qnamespace.h # this header is specified on purpose so AUTOMOC processes it
        global/qnativeinterface.h
        global/qnumeric.cpp global/qnumeric.h global/qnumeric_p.h
        global/qoperatingsystemversion.cpp global/qoperatingsystemversion.h global/qoperatingsystemversion_p.h
        global/qprocessordetection.h
        global/qrandom.cpp global/qrandom.h global/qrandom_p.h
        global/qsimd.cpp global/qsimd.h global/qsimd_p.h
        global/qsysinfo.h
        global/qsystemdetection.h
        global/qtypeinfo.h
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
        io/qfsfileengine.cpp io/qfsfileengine_p.h
        io/qfsfileengine_iterator.cpp io/qfsfileengine_iterator_p.h
        io/qiodevice.cpp io/qiodevice.h io/qiodevice_p.h
        io/qiodevicebase.h
        io/qipaddress.cpp io/qipaddress_p.h
        io/qlockfile.cpp io/qlockfile.h io/qlockfile_p.h
        io/qloggingcategory.cpp io/qloggingcategory.h
        io/qloggingregistry.cpp io/qloggingregistry_p.h
        io/qnoncontiguousbytedevice.cpp io/qnoncontiguousbytedevice_p.h
        io/qresource.cpp io/qresource_p.h
        io/qresource_iterator.cpp io/qresource_iterator_p.h
        io/qsavefile.cpp io/qsavefile.h
        io/qstandardpaths.cpp io/qstandardpaths.h
        io/qstorageinfo.cpp io/qstorageinfo.h io/qstorageinfo_p.h
        io/qtemporarydir.cpp io/qtemporarydir.h
        io/qtemporaryfile.cpp io/qtemporaryfile.h io/qtemporaryfile_p.h
        io/qurl.cpp io/qurl.h io/qurl_p.h
        io/qurlidna.cpp
        io/qurlquery.cpp io/qurlquery.h
        io/qurlrecode.cpp
        itemmodels/qabstractitemmodel.cpp itemmodels/qabstractitemmodel.h itemmodels/qabstractitemmodel_p.h
        itemmodels/qitemselectionmodel.cpp itemmodels/qitemselectionmodel.h itemmodels/qitemselectionmodel_p.h
        kernel/qabstracteventdispatcher.cpp kernel/qabstracteventdispatcher.h kernel/qabstracteventdispatcher_p.h
        kernel/qabstractnativeeventfilter.cpp kernel/qabstractnativeeventfilter.h
        kernel/qassociativeiterable.cpp kernel/qassociativeiterable.h
        kernel/qbasictimer.cpp kernel/qbasictimer.h
        kernel/qbindingstorage.h
        kernel/qcoreapplication.cpp kernel/qcoreapplication.h kernel/qcoreapplication_p.h
        kernel/qcoreapplication_platform.h
        kernel/qcorecmdlineargs_p.h
        kernel/qcoreevent.cpp kernel/qcoreevent.h
        kernel/qcoreglobaldata.cpp kernel/qcoreglobaldata_p.h
        kernel/qdeadlinetimer.cpp kernel/qdeadlinetimer.h kernel/qdeadlinetimer_p.h
        kernel/qelapsedtimer.cpp kernel/qelapsedtimer.h
        kernel/qeventloop.cpp kernel/qeventloop.h
        kernel/qfunctions_p.h
        kernel/qiterable.cpp kernel/qiterable.h kernel/qiterable_p.h
        kernel/qmath.cpp kernel/qmath.h
        kernel/qmetacontainer.cpp kernel/qmetacontainer.h
        kernel/qmetaobject.cpp kernel/qmetaobject.h kernel/qmetaobject_p.h
        kernel/qmetaobject_moc_p.h
        kernel/qmetaobjectbuilder.cpp kernel/qmetaobjectbuilder_p.h
        kernel/qmetatype.cpp kernel/qmetatype.h kernel/qmetatype_p.h
        kernel/qmimedata.cpp kernel/qmimedata.h
        kernel/qobject.cpp kernel/qobject.h kernel/qobject_p.h
        kernel/qobject_impl.h
        kernel/qobjectcleanuphandler.cpp kernel/qobjectcleanuphandler.h
        kernel/qobjectdefs.h
        kernel/qobjectdefs_impl.h
        kernel/qpointer.cpp kernel/qpointer.h
        kernel/qproperty.cpp kernel/qproperty.h kernel/qproperty_p.h
        kernel/qpropertyprivate.h
        kernel/qsequentialiterable.cpp kernel/qsequentialiterable.h
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
        serialization/qcborcommon.cpp serialization/qcborcommon.h serialization/qcborcommon_p.h
        serialization/qcbordiagnostic.cpp
        serialization/qcbormap.h
        serialization/qcborstream.h
        serialization/qcborvalue.cpp serialization/qcborvalue.h serialization/qcborvalue_p.h
        serialization/qdatastream.cpp serialization/qdatastream.h serialization/qdatastream_p.h
        serialization/qjson_p.h
        serialization/qjsonarray.cpp serialization/qjsonarray.h
        serialization/qjsoncbor.cpp
        serialization/qjsondocument.cpp serialization/qjsondocument.h
        serialization/qjsonobject.cpp serialization/qjsonobject.h
        serialization/qjsonparser.cpp serialization/qjsonparser_p.h
        serialization/qjsonvalue.cpp serialization/qjsonvalue.h
        serialization/qjsonwriter.cpp serialization/qjsonwriter_p.h
        serialization/qtextstream.cpp serialization/qtextstream.h serialization/qtextstream_p.h
        serialization/qxmlstream.cpp serialization/qxmlstream.h serialization/qxmlstream_p.h
        serialization/qxmlstreamgrammar.cpp serialization/qxmlstreamgrammar_p.h
        serialization/qxmlstreamparser_p.h
        serialization/qxmlutils.cpp serialization/qxmlutils_p.h
        text/qanystringview.h
        text/qbytearray.cpp text/qbytearray.h text/qbytearray_p.h
        text/qbytearrayalgorithms.h
        text/qbytearraylist.cpp text/qbytearraylist.h
        text/qbytearraymatcher.cpp text/qbytearraymatcher.h
        text/qbytearrayview.h
        text/qbytedata_p.h
        text/qchar.h
        text/qcollator.cpp text/qcollator.h text/qcollator_p.h
        text/qdoublescanprint_p.h
        text/qlocale.cpp text/qlocale.h text/qlocale_p.h
        text/qlocale_data_p.h
        text/qlocale_tools.cpp text/qlocale_tools_p.h
        text/qstring.cpp text/qstring.h
        text/qstringalgorithms.h text/qstringalgorithms_p.h
        text/qstringbuilder.cpp text/qstringbuilder.h
        text/qstringconverter.cpp text/qstringconverter.h text/qstringconverter_p.h
        text/qstringiterator_p.h
        text/qstringlist.cpp text/qstringlist.h
        text/qstringliteral.h
        text/qstringmatcher.h
        text/qstringtokenizer.cpp text/qstringtokenizer.h
        text/qstringview.cpp text/qstringview.h
        text/qtextboundaryfinder.cpp text/qtextboundaryfinder.h
        text/qunicodetables_p.h
        text/qunicodetools.cpp text/qunicodetools_p.h
        text/qutf8stringview.h
        text/qvsnprintf.cpp
        thread/qmutex.h
        thread/qreadwritelock.h
        thread/qrunnable.cpp thread/qrunnable.h
        thread/qthread.cpp thread/qthread.h
        thread/qthreadstorage.h
        thread/qwaitcondition.h thread/qwaitcondition_p.h
        time/qcalendar.cpp time/qcalendar.h
        time/qcalendarbackend_p.h
        time/qcalendarmath_p.h
        time/qdatetime.cpp time/qdatetime.h time/qdatetime_p.h
        time/qgregoriancalendar.cpp time/qgregoriancalendar_p.h
        time/qjuliancalendar.cpp time/qjuliancalendar_p.h
        time/qmilankoviccalendar.cpp time/qmilankoviccalendar_p.h
        time/qromancalendar.cpp time/qromancalendar_p.h
        time/qromancalendar_data_p.h
        tools/qalgorithms.h
        tools/qarraydata.cpp tools/qarraydata.h
        tools/qarraydataops.h
        tools/qarraydatapointer.h
        tools/qbitarray.cpp tools/qbitarray.h
        tools/qcache.h
        tools/qcontainerfwd.h
        tools/qcontainertools_impl.h
        tools/qcontiguouscache.cpp tools/qcontiguouscache.h
        tools/qcryptographichash.cpp tools/qcryptographichash.h
        tools/qduplicatetracker_p.h
        tools/qflatmap_p.h
        tools/qfreelist.cpp tools/qfreelist_p.h
        tools/qhash.cpp tools/qhash.h
        tools/qhashfunctions.h
        tools/qiterator.h
        tools/qline.cpp tools/qline.h
        tools/qlist.h
        tools/qmakearray_p.h
        tools/qmap.h
        tools/qmargins.cpp tools/qmargins.h
        tools/qmessageauthenticationcode.cpp tools/qmessageauthenticationcode.h
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
        tools/qstack.h
        tools/qtaggedpointer.h
        tools/qtools_p.h
        tools/qvarlengtharray.h
        tools/qvector.h
        tools/qversionnumber.cpp tools/qversionnumber.h
    DEFINES
        QT_NO_FOREACH
        QT_NO_USING_NAMESPACE
    INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_BINARY_DIR}/global" # special case
        "${CMAKE_CURRENT_BINARY_DIR}/kernel" # for moc_qobject.cpp to be found by qobject.cpp
        # ../3rdparty/md4 # special case remove
        # ../3rdparty/md5 # special case remove
        # ../3rdparty/sha3 # special case remove
        ../3rdparty/tinycbor/src
    LIBRARIES
        Qt::GlobalConfigPrivate # special case
    PRECOMPILED_HEADER
        "global/qt_pch.h"
    PUBLIC_LIBRARIES  # special case:
        Qt::Platform # special case:
    # special case begin
    # Generated in QtBaseGlobalTargets
    EXTRA_CMAKE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Qt6CTestMacros.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/Qt6CoreConfigureFileTemplate.in"
    ${corelib_extra_cmake_files}
    # special case end
)

qt_update_ignore_pch_source(Core kernel/qmetatype.cpp )

# special case begin
add_dependencies(Core qmodule_pri)

if (NOT QT_NAMESPACE STREQUAL "")
    target_compile_definitions(Core PUBLIC "QT_NAMESPACE=${QT_NAMESPACE}")
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
set_source_files_properties(kernel/qobject.cpp kernel/qobject.h kernel/qobject_p.h
                            PROPERTIES SKIP_AUTOMOC TRUE)

qt_manual_moc(qobject_moc_files
               OUTPUT_MOC_JSON_FILES core_qobject_metatypes_json_list
               kernel/qobject.h)
# The moc file is included directly by qobject.cpp
set_source_files_properties(${qobject_moc_files} PROPERTIES HEADER_FILE_ONLY ON)

set(core_metatype_args MANUAL_MOC_JSON_FILES ${core_qobject_metatypes_json_list})

if(QT_WILL_INSTALL)
    set(metatypes_install_dir ${INSTALL_LIBDIR}/metatypes)
    list(APPEND core_metatype_args
        __QT_INTERNAL_INSTALL __QT_INTERNAL_INSTALL_DIR "${metatypes_install_dir}")
endif()
# Use qt6_extract_metatypes instead of GENERATE_METATYPES so that we can manually pass the
# additional json files.
qt6_extract_metatypes(Core ${core_metatype_args})

set_property(TARGET Core APPEND PROPERTY
    PUBLIC_HEADER "${CMAKE_CURRENT_BINARY_DIR}/global/qconfig.h")
set_property(TARGET Core APPEND PROPERTY
    PRIVATE_HEADER "${CMAKE_CURRENT_BINARY_DIR}/global/qconfig_p.h")

# Find ELF interpreter and define a macro for that:
if (LINUX AND NOT CMAKE_CROSSCOMPILING AND BUILD_SHARED_LIBS)
    if (NOT DEFINED ELF_INTERPRETER)
        execute_process(COMMAND ${CMAKE_COMMAND} -E env LC_ALL=C readelf -l /bin/ls
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
# special case end

qt_internal_add_simd_part(Core SIMD mips_dsp
    SOURCES
        ../gui/painting/qt_mips_asm_dsp_p.h
        text/qstring_mips_dsp_asm.S
)

if(QT_FEATURE_reduce_relocations AND UNIX AND GCC)
    target_link_options(Core PRIVATE
                        "LINKER:--dynamic-list=${CMAKE_CURRENT_LIST_DIR}/QtCore.dynlist")
endif()

if(ANDROID)
    set_property(TARGET Core APPEND PROPERTY QT_ANDROID_BUNDLED_JAR_DEPENDENCIES
        jar/Qt${QtBase_VERSION_MAJOR}Android.jar # special case
    )
    set_property(TARGET Core APPEND PROPERTY QT_ANDROID_LIB_DEPENDENCIES
        plugins/platforms/libplugins_platforms_qtforandroid.so
    )
    set_property(TARGET Core APPEND PROPERTY QT_ANDROID_PERMISSIONS
        android.permission.INTERNET android.permission.WRITE_EXTERNAL_STORAGE
    )
endif()

#### Keys ignored in scope 1:.:.:corelib.pro:<TRUE>:
# CMAKE_DISABLED_FEATURES = "$$join(QT_DISABLED_FEATURES, "$$escape_expand(\\n)    ")"
# CMAKE_HOST_DATA_DIR = "$$cmakeRelativePath($$[QT_HOST_DATA/src], $$[QT_INSTALL_PREFIX])"
# CMAKE_INSTALL_DATA_DIR = "$$cmakeRelativePath($$[QT_HOST_DATA], $$[QT_INSTALL_PREFIX])"
# HOST_BINS = "$$[QT_HOST_BINS]"
# INSTALLS = "ctest_qt5_module_files" "cmake_qt5_umbrella_module_files"
# MODULE = "core"
# MODULE_CONFIG = "moc" "resources"
# QMAKE_PKGCONFIG_VARIABLES = "host_bins" "qt_conf"
# QMAKE_SUBSTITUTES = "ctest_macros_file" "cmake_umbrella_config_file" "cmake_umbrella_config_module_location" "cmake_umbrella_config_module_location_for_install" "cmake_umbrella_config_version_file" "cmake_extras_mkspec_dir" "cmake_extras_mkspec_dir_for_install"
# cmake_extras_mkspec_dir.input = "$$PWD/Qt5CoreConfigExtrasMkspecDir.cmake.in"
# cmake_extras_mkspec_dir.output = "$$DESTDIR/cmake/Qt5Core/Qt5CoreConfigExtrasMkspecDir.cmake"
# cmake_extras_mkspec_dir_for_install.input = "$$PWD/Qt5CoreConfigExtrasMkspecDirForInstall.cmake.in"
# cmake_extras_mkspec_dir_for_install.output = "$$DESTDIR/cmake/install/Qt5Core/Qt5CoreConfigExtrasMkspecDir.cmake"
# cmake_qt5_umbrella_module_files.files = "$$cmake_umbrella_config_file.output" "$$cmake_umbrella_config_version_file.output" "$$cmake_umbrella_config_module_location_for_install.output"
# cmake_qt5_umbrella_module_files.path = "$$[QT_INSTALL_LIBS]/cmake/Qt5"
# cmake_umbrella_config_file.input = "$$PWD/Qt5Config.cmake.in"
# cmake_umbrella_config_file.output = "$$DESTDIR/cmake/Qt5/Qt5Config.cmake"
# cmake_umbrella_config_module_location.input = "$$PWD/Qt5ModuleLocation.cmake.in"
# cmake_umbrella_config_module_location.output = "$$DESTDIR/cmake/Qt5/Qt5ModuleLocation.cmake"
# cmake_umbrella_config_module_location_for_install.input = "$$PWD/Qt5ModuleLocationForInstall.cmake.in"
# cmake_umbrella_config_module_location_for_install.output = "$$DESTDIR/cmake/install/Qt5/Qt5ModuleLocation.cmake"
# cmake_umbrella_config_version_file.input = "$$PWD/../../mkspecs/features/data/cmake/Qt5ConfigVersion.cmake.in"
# cmake_umbrella_config_version_file.output = "$$DESTDIR/cmake/Qt5/Qt5ConfigVersion.cmake"
# ctest_macros_file.CONFIG = "verbatim"
# ctest_macros_file.input = "$$PWD/Qt5CTestMacros.cmake"
# ctest_macros_file.output = "$$DESTDIR/cmake/Qt5Core/Qt5CTestMacros.cmake"
# ctest_qt5_module_files.files = "$$ctest_macros_file.output" "$$cmake_extras_mkspec_dir_for_install.output"
# ctest_qt5_module_files.path = "$$[QT_INSTALL_LIBS]/cmake/Qt5Core"
# host_bins.name = "host_bins"
# host_bins.variable = "HOST_BINS"
# qt_conf.name = "qt_config"
# qt_conf.variable = "QT_CONFIG"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:corelib.pro:QT_FEATURE_gc_binaries:
# MODULE_CONFIG = "gc_binaries"

# special case begin
# remove because it's handled manually
#qt_internal_extend_target(Core CONDITION NOT QT_NAMESPACE_ISEMPTY
#    PUBLIC_DEFINES
#        QT_NAMESPACE=
#)
# special case end

qt_internal_extend_target(Core CONDITION ANDROID
    SOURCES
        kernel/qsharedmemory_android.cpp
        kernel/qsystemsemaphore_android.cpp
    DEFINES
        LIBS_SUFFIX="_${ANDROID_ABI}.so" # special case
)

qt_internal_extend_target(Core CONDITION MSVC AND (TEST_architecture_arch STREQUAL "i386")
    LINK_OPTIONS
        "/BASE:0x67000000"
)

# QtCore can't be compiled with -Wl,-no-undefined because it uses the
# "environ" variable and FreeBSD does not include a weak symbol for it
# in libc.
qt_internal_extend_target(Core CONDITION FREEBSD
    LINK_OPTIONS
      "LINKER:--warn-unresolved-symbols"
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
        thread/qmutex_win.cpp
        thread/qwaitcondition_win.cpp
    LIBRARIES
        synchronization
)

qt_internal_extend_target(Core CONDITION WIN32
    SOURCES
        global/qoperatingsystemversion_win.cpp global/qoperatingsystemversion_win_p.h
        io/qfilesystemengine_win.cpp
        io/qfsfileengine_win.cpp
        io/qlockfile_win.cpp
        io/qstandardpaths_win.cpp
        io/qstorageinfo_win.cpp
        io/qwindowspipereader.cpp io/qwindowspipereader_p.h
        io/qwindowspipewriter.cpp io/qwindowspipewriter_p.h
        kernel/qcoreapplication_win.cpp
        kernel/qelapsedtimer_win.cpp
        kernel/qeventdispatcher_win.cpp kernel/qeventdispatcher_win_p.h
        kernel/qfunctions_winrt_p.h
        kernel/qsharedmemory_win.cpp
        kernel/qsystemsemaphore_win.cpp
        kernel/qwineventnotifier.cpp kernel/qwineventnotifier.h kernel/qwineventnotifier_p.h
        kernel/qwinregistry.cpp kernel/qwinregistry_p.h
        plugin/qsystemlibrary.cpp plugin/qsystemlibrary_p.h
        thread/qthread_win.cpp
    # DEFINES # special case: remove
    LIBRARIES
        advapi32
        kernel32
        netapi32
        ole32
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
        kernel/qelapsedtimer_mac.cpp
        kernel/qeventdispatcher_cf.mm kernel/qeventdispatcher_cf_p.h
    LIBRARIES
        ${FWCoreFoundation}
        ${FWFoundation}
)

qt_internal_extend_target(Core CONDITION MACOS
    LIBRARIES
        ${FWAppKit}
        ${FWApplicationServices}
        ${FWCoreServices}
        ${FWSecurity}
    PUBLIC_LIBRARIES
        ${FWDiskArbitration}
        ${FWIOKit}
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

# Workaround for QTBUG-90353
# Remove if MinGW (our only supported gcc version < 9) is no longer supported
qt_internal_extend_target(Core CONDITION GCC AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0.0")
    PUBLIC_COMPILE_OPTIONS -Wno-error=invalid-offsetof
)

#### Keys ignored in scope 14:.:.:corelib.pro:pathIsAbsolute(_ss_CMAKE_HOST_DATA_DIR):
# CMAKE_HOST_DATA_DIR = "$$[QT_HOST_DATA/src]/"
# CMAKE_HOST_DATA_DIR_IS_ABSOLUTE = "True"

#### Keys ignored in scope 15:.:.:corelib.pro:pathIsAbsolute(_ss_CMAKE_INSTALL_DATA_DIR):
# CMAKE_INSTALL_DATA_DIR = "$$[QT_HOST_DATA]/"
# CMAKE_INSTALL_DATA_DIR_IS_ABSOLUTE = "True"

#### Keys ignored in scope 20:.:global:global/global.pri:QT_CPU_FEATURES__ss_QT_ARCH___contains___f16c:
# f16c_cxx = "true"

#### Keys ignored in scope 22:.:global:global/global.pri:CLANG OR intel_icl OR ICC:
# f16c_cxx = "false"

#### Keys ignored in scope 24:.:global:global/global.pri:GCC AND f16c AND x86SimdAlways:
# f16c_cxx = "true"

#### Keys ignored in scope 26:.:global:global/global.pri:MSVC AND QT_CPU_FEATURES__ss_QT_ARCH___contains___avx:
# f16c_cxx = "true"

#### Keys ignored in scope 27:.:global:global/global.pri:else:
# f16c_cxx = "false"

# qt_internal_extend_target(Core CONDITION _ss_f16c_cxx ... # special case remove: Handled manually

# special case begin
# Remove this because it's handled manually
#qt_internal_extend_target(Core CONDITION NOT _x_-armcc_x_ AND NOT cross_compile AND NOT static AND (hurd_x_ OR linux_x_)
#    DEFINES
#        ELF_INTERPRETER=\\\"=Creadelf-l/bin/ls|perl-n-e\'['quote', ['if', ['/program', 'interpreter:', ['.*'], ']/'], '{', 'print', '$1;', '}']]\'\\\"
#    LINK_OPTIONS
#        "-Wl,-e,qt_core_boilerplate"
#)
# special case end

qt_internal_extend_target(Core CONDITION LINUX AND QT_BUILD_SHARED_LIBS
    SOURCES
        global/minimum-linux_p.h
)

#### Keys ignored in scope 34:.:global:global/global.pri:precompile_header:
# NO_PCH_ASM = "global/minimum-linux.S"
# QMAKE_EXTRA_COMPILERS = "no_pch_assembler"
# no_pch_assembler.commands = "$$QMAKE_CC" "-c" "$(CFLAGS)" "$(INCPATH)" "${QMAKE_FILE_IN}" "-o" "${QMAKE_FILE_OUT}"
# no_pch_assembler.dependency_type = "TYPE_C"
# no_pch_assembler.input = "NO_PCH_ASM"
# no_pch_assembler.name = "compiling[no_pch]" "${QMAKE_FILE_IN}"
# no_pch_assembler.output = "${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${firstQMAKE_EXT_OBJ}"

#### Keys ignored in scope 35:.:global:global/global.pri:silent:
# no_pch_assembler.commands = "@echo" "compiling[no_pch]" "${QMAKE_FILE_IN}" "&&"

qt_internal_extend_target(Core CONDITION LINUX AND NOT precompile_header AND QT_BUILD_SHARED_LIBS
    SOURCES
        global/minimum-linux.S
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_slog2
    LIBRARIES
        Slog2::Slog2
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_journald
    LIBRARIES
        PkgConfig::Libsystemd
)

set(core_version_tagging_files global/qversiontagging.cpp global/qversiontagging.h)
target_sources(Core PRIVATE ${core_version_tagging_files})

# Disable LTO, as the symbols disappear somehow under GCC
# (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=48200)
if(GCC AND FEATURE_ltcg)
    set_source_files_properties(${core_version_tagging_files}
                                PROPERTIES COMPILE_OPTIONS "-fno-lto")
endif()

qt_internal_extend_target(Core CONDITION UNIX
    SOURCES
        io/qfilesystemengine_unix.cpp
        io/qfilesystemiterator_unix.cpp
        io/qfsfileengine_unix.cpp
        io/qlockfile_unix.cpp
        kernel/qcore_unix.cpp kernel/qcore_unix_p.h
        kernel/qeventdispatcher_unix.cpp kernel/qeventdispatcher_unix_p.h
        kernel/qpoll_p.h
        kernel/qtimerinfo_unix.cpp kernel/qtimerinfo_unix_p.h
        thread/qthread_unix.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_thread
    SOURCES
        thread/qatomic.cpp thread/qatomic.h
        thread/qatomic_bootstrap.h
        thread/qatomic_cxx11.h
        thread/qbasicatomic.h
        thread/qfutex_p.h
        thread/qgenericatomic.h
        thread/qlocking_p.h
        thread/qmutex.cpp thread/qmutex_p.h
        thread/qorderedmutexlocker_p.h
        thread/qreadwritelock.cpp thread/qreadwritelock_p.h
        thread/qsemaphore.cpp thread/qsemaphore.h
        thread/qthread_p.h
        thread/qthreadpool.cpp thread/qthreadpool.h thread/qthreadpool_p.h
        thread/qthreadstorage.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_thread AND UNIX
    SOURCES
        thread/qwaitcondition_unix.cpp
)

qt_internal_extend_target(Core CONDITION APPLE AND QT_FEATURE_thread
    SOURCES
        thread/qmutex_mac.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_thread AND UNIX AND NOT APPLE AND NOT LINUX
    SOURCES
        thread/qmutex_unix.cpp
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

qt_internal_extend_target(Core CONDITION ICC
    COMPILE_OPTIONS
        -fp-model
        strict
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_system_zlib
    LIBRARIES
        WrapZLIB::WrapZLIB
)

qt_internal_extend_target(Core CONDITION NOT QT_FEATURE_system_zlib
    SOURCES
        ../3rdparty/zlib/src/adler32.c
        ../3rdparty/zlib/src/compress.c
        ../3rdparty/zlib/src/crc32.c
        ../3rdparty/zlib/src/deflate.c
        ../3rdparty/zlib/src/gzclose.c
        ../3rdparty/zlib/src/gzlib.c
        ../3rdparty/zlib/src/gzread.c
        ../3rdparty/zlib/src/gzwrite.c
        ../3rdparty/zlib/src/infback.c
        ../3rdparty/zlib/src/inffast.c
        ../3rdparty/zlib/src/inflate.c
        ../3rdparty/zlib/src/inftrees.c
        ../3rdparty/zlib/src/trees.c
        ../3rdparty/zlib/src/uncompr.c
        ../3rdparty/zlib/src/zutil.c
    INCLUDE_DIRECTORIES
        ../3rdparty/zlib/src
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_commandlineparser
    SOURCES
        tools/qcommandlineoption.cpp tools/qcommandlineoption.h
        tools/qcommandlineparser.cpp tools/qcommandlineparser.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_backtrace
    LIBRARIES
        WrapBacktrace::WrapBacktrace
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_system_doubleconversion
    LIBRARIES
        WrapDoubleConversion::WrapDoubleConversion
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
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_system_libb2
    LIBRARIES
        Libb2::Libb2
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_easingcurve
    SOURCES
        tools/qeasingcurve.cpp tools/qeasingcurve.h
        tools/qtimeline.cpp tools/qtimeline.h
)

qt_internal_extend_target(Core CONDITION UNIX AND NOT HAIKU AND NOT INTEGRITY AND NOT VXWORKS AND NOT WASM AND (NOT ICC OR NOT MACOS)
    LIBRARIES
        m
)

qt_internal_extend_target(Core CONDITION APPLE AND NOT NACL
    SOURCES
        text/qlocale_mac.mm
)

qt_internal_extend_target(Core CONDITION UNIX AND (NACL OR NOT APPLE)
    SOURCES
        text/qlocale_unix.cpp
)

qt_internal_extend_target(Core CONDITION WIN32
    SOURCES
        text/qlocale_win.cpp
)

# On MS-Win, clang has two flavors, one of which immitates MSVC (so claims to be it)
qt_internal_extend_target(Core CONDITION WIN32 AND MSVC AND NOT CLANG
    LIBRARIES
        runtimeobject
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

qt_internal_extend_target(Core CONDITION QT_FEATURE_regularexpression
    SOURCES
        text/qregularexpression.cpp text/qregularexpression.h
    LIBRARIES
        WrapPCRE2::WrapPCRE2
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_hijricalendar
    SOURCES
        time/qhijricalendar.cpp time/qhijricalendar_p.h
        time/qhijricalendar_data_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_islamiccivilcalendar
    SOURCES
        time/qislamiccivilcalendar.cpp time/qislamiccivilcalendar_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_jalalicalendar
    SOURCES
        time/qjalalicalendar.cpp time/qjalalicalendar_p.h
        time/qjalalicalendar_data_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_timezone
    SOURCES
        time/qtimezone.cpp time/qtimezone.h
        time/qtimezoneprivate.cpp time/qtimezoneprivate_p.h
        time/qtimezoneprivate_data_p.h
)

qt_internal_extend_target(Core CONDITION APPLE AND QT_FEATURE_timezone AND NOT NACL
    SOURCES
        time/qtimezoneprivate_mac.mm
)

qt_internal_extend_target(Core CONDITION ANDROID AND QT_FEATURE_timezone AND NOT ANDROID_EMBEDDED AND (NACL OR NOT APPLE)
    SOURCES
        time/qtimezoneprivate_android.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_timezone AND UNIX AND (ANDROID_EMBEDDED OR NOT ANDROID) AND (NACL OR NOT APPLE)
    SOURCES
        time/qtimezoneprivate_tz.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_icu AND QT_FEATURE_timezone AND UNIX AND (ANDROID_EMBEDDED OR NOT ANDROID) AND (NACL OR NOT APPLE)
    SOURCES
        time/qtimezoneprivate_icu.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_icu AND QT_FEATURE_timezone AND WIN32 AND (ANDROID_EMBEDDED OR NOT ANDROID) AND (NACL OR NOT APPLE)
    SOURCES
        time/qtimezoneprivate_icu.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_timezone AND WIN32 AND NOT QT_FEATURE_icu AND (ANDROID_EMBEDDED OR NOT ANDROID) AND (NACL OR NOT APPLE)
    SOURCES
        time/qtimezoneprivate_win.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_datetimeparser
    SOURCES
        time/qdatetimeparser.cpp time/qdatetimeparser_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_zstd
    LIBRARIES
        ZSTD::ZSTD
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

qt_internal_extend_target(Core CONDITION APPLE AND QT_FEATURE_settings AND NOT NACL
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
        io/forkfd_qt.cpp
    INCLUDE_DIRECTORIES
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

qt_internal_extend_target(Core CONDITION UNIX AND NOT APPLE
    SOURCES
        kernel/qelapsedtimer_unix.cpp
)

qt_internal_extend_target(Core CONDITION ANDROID AND NOT ANDROID_EMBEDDED
    SOURCES
        io/qstandardpaths_android.cpp
        io/qstorageinfo_unix.cpp
        kernel/qjnienvironment.cpp kernel/qjnienvironment.h
        kernel/qjniobject.cpp kernel/qjniobject.h
        kernel/qjnihelpers.cpp kernel/qjnihelpers_p.h
        platform/android/qandroidextras_p.h platform/android/qandroidextras.cpp
        platform/android/qandroidnativeinterface.cpp
)

qt_internal_extend_target(Core CONDITION HAIKU AND (ANDROID_EMBEDDED OR NOT ANDROID)
    SOURCES
        io/qstandardpaths_haiku.cpp
        io/qstorageinfo_unix.cpp
    PUBLIC_LIBRARIES
        be
)

qt_internal_extend_target(Core CONDITION UNIX AND NOT APPLE AND NOT HAIKU AND (ANDROID_EMBEDDED OR NOT ANDROID)
    SOURCES
        io/qstandardpaths_unix.cpp
        io/qstorageinfo_unix.cpp
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
        plugin/qelfparser_p.cpp plugin/qelfparser_p.h
        plugin/qlibrary.cpp plugin/qlibrary.h plugin/qlibrary_p.h
        plugin/qmachparser.cpp plugin/qmachparser_p.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_library AND UNIX
    SOURCES
        plugin/qlibrary_unix.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_library AND WIN32
    SOURCES
        plugin/qlibrary_win.cpp
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_dlopen AND QT_FEATURE_library
    LIBRARIES
        ${CMAKE_DL_LIBS}
)

qt_internal_extend_target(Core CONDITION APPLE AND (IOS OR TVOS)
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

qt_internal_extend_target(Core CONDITION QT_FEATURE_clock_gettime AND UNIX
    LIBRARIES
        WrapRt::WrapRt
)

qt_internal_extend_target(Core CONDITION UNIX AND NOT ANDROID
    SOURCES
        kernel/qsharedmemory_posix.cpp
        kernel/qsharedmemory_systemv.cpp
        kernel/qsharedmemory_unix.cpp
        kernel/qsystemsemaphore_posix.cpp
        kernel/qsystemsemaphore_systemv.cpp
        kernel/qsystemsemaphore_unix.cpp
)

qt_internal_extend_target(Core CONDITION VXWORKS
    SOURCES
        kernel/qfunctions_vxworks.cpp kernel/qfunctions_vxworks.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_cborstreamreader
    SOURCES
        serialization/qcborstreamreader.cpp serialization/qcborstreamreader.h
)

qt_internal_extend_target(Core CONDITION QT_FEATURE_cborstreamwriter
    SOURCES
        serialization/qcborstreamwriter.cpp serialization/qcborstreamwriter.h
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

#### Keys ignored in scope 171:.:mimetypes:mimetypes/mimetypes.pri:QT_FEATURE_mimetype:
# MIME_DATABASE = "mimetypes/mime/packages/freedesktop.org.xml"
# OTHER_FILES = "$$MIME_DATABASE"

# special case begin
# qt_internal_extend_target(Core CONDITION QT_FEATURE_mimetype AND QT_FEATURE_mimetype_database
#        INCLUDE_DIRECTORIES
#        .rcc
#)
# special case end

#### Keys ignored in scope 172:.:mimetypes:mimetypes/mimetypes.pri:QT_FEATURE_mimetype_database:
# QMAKE_EXTRA_COMPILERS = "mimedb"
# mimedb.commands = "${QMAKE_FILE_IN}" ">" "${QMAKE_FILE_OUT}"
# mimedb.depends = "$$PWD/mime/generate.pl"
# mimedb.input = "MIME_DATABASE"
# mimedb.output = "$$outpath/qmimeprovider_database.cpp"
# mimedb.variable_out = "INCLUDED_SOURCES"

#### Keys ignored in scope 173:.:mimetypes:mimetypes/mimetypes.pri:ANDROID:
# outpath = "$$outpath/$${QT_ARCH}"

#### Keys ignored in scope 175:.:mimetypes:mimetypes/mimetypes.pri:(CMAKE_BUILD_TYPE STREQUAL Debug):
# outpath = "$$outpath/debug"

#### Keys ignored in scope 176:.:mimetypes:mimetypes/mimetypes.pri:else:
# outpath = "$$outpath/release"

#### Keys ignored in scope 177:.:mimetypes:mimetypes/mimetypes.pri:MAKEFILE_GENERATOR___equals___MSVC.NET OR MAKEFILE_GENERATOR___equals___MSBUILD OR QMAKE_SH_ISEMPTY:
# mimedb.commands = "cmd" "/c" "$$shell_path($$PWD/mime/generate.bat)"
# mimedb.depends = "$$PWD/mime/generate.bat" "$$PWD/mime/hexdump.ps1"

#### Keys ignored in scope 178:.:mimetypes:mimetypes/mimetypes.pri:else:
# mimedb.commands = "perl" "$${mimedb.depends}"

#### Keys ignored in scope 179:.:mimetypes:mimetypes/mimetypes.pri:QT_FEATURE_zstd:
# mimedb.commands = "--zstd"
# Resources:
# special case begin
# We can't specify the resources directly as we have unit tests that
# depend on these as well and we no longer support the use of hand
# edited qrc files.
if(QT_FEATURE_mimetype AND QT_FEATURE_mimetype_database)
    include(${CMAKE_CURRENT_SOURCE_DIR}/mimetypes/mimetypes_resources.cmake)

# Generate qmimeprovider_database.cpp
    set(qmimeprovider_db_output_dir "${CMAKE_CURRENT_BINARY_DIR}/.rcc")
    set(qmimeprovider_db_output "${qmimeprovider_db_output_dir}/qmimeprovider_database.cpp")
    if(CMAKE_VERSION VERSION_LESS 3.18 OR QT_AVOID_CMAKE_ARCHIVING_API)
        set(command_args "")
        set(mime_dir "${CMAKE_CURRENT_SOURCE_DIR}/mimetypes/mime")
        set(command_depends "${mime_dir}/generate.pl" "${corelib_mimetypes_resource_file}")
        if (MSVC)
            list(APPEND command_args "${mime_dir}/generate.bat")
            list(APPEND command_depends "${mime_dir}/generate.bat" "${mime_dir}/hexdump.ps1" )
        else()
            file(MAKE_DIRECTORY ${qmimeprovider_db_output_dir})
            list(APPEND command_args perl "${mime_dir}/generate.pl" )
        endif()

        if (QT_FEATURE_zstd)
            list(APPEND command_args "--zstd")
        endif()

        list(APPEND command_args "${corelib_mimetypes_resource_file}" ">" "${qmimeprovider_db_output}")

        add_custom_command(OUTPUT "${qmimeprovider_db_output}"
            DEPENDS ${command_depends}
            COMMAND ${command_args}
            COMMENT "Generating ${qmimeprovider_db_output}"
        )
    else()
        if(QT_FEATURE_zstd)
            if(NOT QT_CMAKE_ZSTD_SUPPORT)
                message(FATAL_ERROR
                    "CMake was not built with zstd support. "
                    "Rebuild CMake or set QT_AVOID_CMAKE_ARCHIVING_API=ON.")
            endif()
            set(qmime_db_compression Zstd)
            string(APPEND qmime_db_content "#define MIME_DATABASE_IS_ZSTD\n")
        else()
            set(qmime_db_compression GZip)
            string(APPEND qmime_db_content "#define MIME_DATABASE_IS_GZIP\n")
        endif()

        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.rcc")
        get_filename_component(mime_types_resource_file "${corelib_mimetypes_resource_file}" NAME)

        set(mimetypes_resource_file_minified "${CMAKE_CURRENT_BINARY_DIR}/.rcc/${mime_types_resource_file}")

        set(mimetypes_resfile_timestamp_file "${CMAKE_CURRENT_BINARY_DIR}/.rcc/${mime_types_resource_file}.timestamp")
        file(TIMESTAMP "${corelib_mimetypes_resource_file}" mimetypes_resfile_timestamp)

        set(compute_db_archive ON)
        if (EXISTS "${mimetypes_resfile_timestamp_file}")
            file(READ "${mimetypes_resfile_timestamp_file}" old_mimetypes_resfile_timestamp)
            if ("${mimetypes_resfile_timestamp}" STREQUAL "${old_mimetypes_resfile_timestamp}")
                set(compute_db_archive OFF)
            endif()
        endif()

        if (compute_db_archive)
            find_program(xmlstarlet NAMES xmlstarlet xml)
            if (xmlstarlet)
                execute_process(
                    COMMAND "${xmlstarlet}" sel -D -B -t -c / "${corelib_mimetypes_resource_file}"
                    OUTPUT_FILE "${mimetypes_resource_file_minified}"
                    RESULT_VARIABLE failed_to_minify
                )
                if (NOT failed_to_minify)
                    set(corelib_mimetypes_resource_file "${mimetypes_resource_file_minified}")
                endif()
            else()
                message(STATUS "xmlstarlet command was not found. ${mime_types_resource_file} will not be minified.")
            endif()

            if (CMAKE_VERSION GREATER_EQUAL 3.19)
                set(additional_file_archive_create_parameters COMPRESSION_LEVEL 9)
            endif()

            file(ARCHIVE_CREATE OUTPUT "${qmimeprovider_db_output}.archive"
                PATHS "${corelib_mimetypes_resource_file}"
                FORMAT raw
                COMPRESSION ${qmime_db_compression}
                ${additional_file_archive_create_parameters}
            )
            file(READ "${qmimeprovider_db_output}.archive" qmime_db_archive HEX)
            file(SIZE "${qmimeprovider_db_output}.archive" qmime_db_archive_size)
            file(SIZE "${corelib_mimetypes_resource_file}" qmime_db_resource_size)
            file(REMOVE ${qmimeprovider_db_output}.archive)

            string(APPEND qmime_db_content "static const unsigned char mimetype_database[] = { ")

            string(REGEX MATCHALL "([a-f0-9][a-f0-9])" qmime_db_hex "${qmime_db_archive}")

            list(TRANSFORM qmime_db_hex PREPEND "0x")
            math(EXPR qmime_db_archive_size "${qmime_db_archive_size} - 1")
            foreach(index RANGE 0 ${qmime_db_archive_size} 12)
                list(APPEND index_list ${index})
            endforeach()
            list(TRANSFORM qmime_db_hex PREPEND "\n " AT ${index_list})
            list(JOIN qmime_db_hex ", " qmime_db_hex_joined)

            string(APPEND qmime_db_content "${qmime_db_hex_joined}")
            string(APPEND qmime_db_content "\n};\n")
            string(APPEND qmime_db_content "static constexpr size_t MimeTypeDatabaseOriginalSize = ${qmime_db_resource_size};\n")

            file(WRITE "${qmimeprovider_db_output}" "${qmime_db_content}")

            file(WRITE "${mimetypes_resfile_timestamp_file}" "${mimetypes_resfile_timestamp}")
        endif()
    endif()

    target_sources(Core PRIVATE ${qmimeprovider_db_output})
    set_source_files_properties(${qmimeprovider_db_output} PROPERTIES HEADER_FILE_ONLY TRUE)
    target_include_directories(Core PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/.rcc")
endif()

# special case end

qt_internal_extend_target(Core CONDITION WASM
    SOURCES
        platform/wasm/qstdweb.cpp platform/wasm/qstdweb_p.h
        kernel/qeventdispatcher_wasm.cpp kernel/qeventdispatcher_wasm_p.h
)

# special case begin
# needs refinement for the other compiler specific cases.
if(TEST_arch_${TEST_architecture_arch}_subarch_f16c)
    # Equivalent to contains(QT_CPU_FEATURES.$$QT_ARCH, f16c)
    qt_internal_extend_target(Core
        DEFINES
            QFLOAT16_INCLUDE_FAST
    )
else()
    qt_internal_add_simd_part(Core SIMD f16c
        SOURCES
            global/qfloat16_f16c.c
    )
endif()

set_source_files_properties(
    thread/qmutex_mac.cpp
    thread/qmutex_unix.cpp
    thread/qmutex_win.cpp
    PROPERTIES HEADER_FILE_ONLY ON)  # special case: These files are included by qmutex.cpp!


# Remove QT_NO_CAST_TO_ASCII to ensure that the symbols are included in the library.
if(WIN32)
    get_target_property(defines Core COMPILE_DEFINITIONS)
    list(REMOVE_ITEM defines QT_NO_CAST_TO_ASCII)
    set_target_properties(Core PROPERTIES COMPILE_DEFINITIONS "${defines}")
endif()

qt_internal_apply_gc_binaries_conditional(Core PUBLIC)

# Add entry-point on platforms that need it. A project can opt-out of using the
# entrypoint by setting the qt_no_entrypoint property to TRUE on a target.
if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    # find_package(Qt6Core) should call find_package(Qt6EntryPointPrivate) so that we can
    # link against EntryPointPrivate. Normally this is handled automatically for deps, but
    # for some reason it doesn't work for the EntryPointPrivate, so we need to add it manually.
    qt_record_extra_qt_package_dependency(Core EntryPointPrivate "${PROJECT_VERSION}")

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
# special case end

qt_internal_create_tracepoints(Core qtcore.tracepoints)
qt_internal_add_docs(Core
    doc/qtcore.qdocconf
)

# special case begin
# Handle qtzlib.pro to create headers for the
# QtZlibPrivate target, equivalent to Qt += zlib-private
if(NOT QT_FEATURE_system_zlib)
    qt_internal_add_module(ZlibPrivate
        INTERNAL_MODULE
        HEADER_MODULE
        NO_CONFIG_HEADER_FILE
    )
endif()

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

# Generated from corelib.pro.

# special case begin
qt_find_package(Threads PROVIDED_TARGETS Threads::Threads)
qt_find_package(WrapDoubleConversion REQUIRED
                                     PROVIDED_TARGETS WrapDoubleConversion::WrapDoubleConversion)

if (NOT WrapDoubleConversion_FOUND)
    message(FATAL_ERROR "Your C library does not provide \
                         sscanf_l/snprintf_l _snprintf_l/_sscanf_l. \
                         You need to use libdouble-conversion for double/string conversion.")
endif()
# special case end

#####################################################################
## Core Module:
#####################################################################

add_qt_module(Core
    QMAKE_MODULE_CONFIG moc resources
    SOURCES
        # special case: remove ../3rdparty/harfbuzz
        codecs/qtextcodec_p.h
        codecs/qutfcodec.cpp codecs/qutfcodec_p.h
        global/archdetect.cpp
        global/qcompilerdetection.h
        global/qendian.cpp global/qendian.h global/qendian_p.h
        global/qflags.h
        global/qfloat16.cpp global/qfloat16.h
        global/qglobal.cpp global/qglobal.h
        global/qglobalstatic.h
        global/qhooks.cpp global/qhooks_p.h
        global/qisenum.h
        global/qlibraryinfo.cpp global/qlibraryinfo.h
        global/qlogging.cpp global/qlogging.h
        global/qmalloc.cpp
        global/qmemory_p.h
        # global/qnamespace.h  # special case
        global/qnumeric.cpp global/qnumeric.h global/qnumeric_p.h
        global/qoperatingsystemversion.cpp global/qoperatingsystemversion.h global/qoperatingsystemversion_p.h
        global/qprocessordetection.h
        global/qrandom.cpp global/qrandom.h global/qrandom_p.h
        global/qsysinfo.h
        global/qsystemdetection.h
        global/qtypeinfo.h
        global/qtypetraits.h
        global/qversiontagging.cpp global/qversiontagging.h  # special case
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
        io/qtldurl.cpp io/qtldurl_p.h
        io/qurl.cpp io/qurl.h io/qurl_p.h
        io/qurlidna.cpp
        io/qurlquery.cpp io/qurlquery.h
        io/qurlrecode.cpp
        io/qurltlds_p.h
        itemmodels/qabstractitemmodel.cpp itemmodels/qabstractitemmodel.h itemmodels/qabstractitemmodel_p.h
        itemmodels/qitemselectionmodel.cpp itemmodels/qitemselectionmodel.h itemmodels/qitemselectionmodel_p.h
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
        # kernel/qobject.cpp kernel/qobject.h kernel/qobject_p.h  # special case
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
        statemachine/qabstractstate.cpp statemachine/qabstractstate.h statemachine/qabstractstate_p.h
        statemachine/qabstracttransition.cpp statemachine/qabstracttransition.h statemachine/qabstracttransition_p.h
        statemachine/qfinalstate.cpp statemachine/qfinalstate.h statemachine/qfinalstate_p.h
        statemachine/qhistorystate.cpp statemachine/qhistorystate.h statemachine/qhistorystate_p.h
        statemachine/qsignaleventgenerator_p.h
        statemachine/qsignaltransition.cpp statemachine/qsignaltransition.h statemachine/qsignaltransition_p.h
        statemachine/qstate.cpp statemachine/qstate.h statemachine/qstate_p.h
        statemachine/qstatemachine.cpp statemachine/qstatemachine.h statemachine/qstatemachine_p.h
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
        tools/qcontainertools_impl.h
        tools/qcontiguouscache.cpp tools/qcontiguouscache.h
        tools/qcryptographichash.cpp tools/qcryptographichash.h
        tools/qdatetime.cpp tools/qdatetime.h tools/qdatetime_p.h
        tools/qdoublescanprint_p.h
        tools/qeasingcurve.cpp tools/qeasingcurve.h
        tools/qfreelist.cpp tools/qfreelist_p.h
        tools/qharfbuzz.cpp tools/qharfbuzz_p.h
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
        tools/qstring_compat.cpp
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
    DEFINES
        QT_NO_FOREACH
        QT_NO_USING_NAMESPACE
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_BINARY_DIR}/global # special case
        # special case: remove ../3rdparty/...
    LIBRARIES
        ZLIB::ZLIB
        # double-conversion # special case
        Qt::GlobalConfigPrivate # special case
        QtHarfBuzz # special case
        Threads::Threads # special case
        tinycbor # special case
        WrapDoubleConversion::WrapDoubleConversion # special case
    PUBLIC_LIBRARIES  # special case:
        Qt::Platform # special case:
    DISABLE_TOOLS_EXPORT # special case:
)

# special case begin
add_dependencies(Core ${QT_CMAKE_EXPORT_NAMESPACE}::moc)
add_dependencies(Core ${QT_CMAKE_EXPORT_NAMESPACE}::rcc)
add_dependencies(CorePrivate ${QT_CMAKE_EXPORT_NAMESPACE}::moc)
add_dependencies(CorePrivate ${QT_CMAKE_EXPORT_NAMESPACE}::rcc)

if (QT_NAMESPACE STREQUAL "")
else()
    target_compile_definitions(Core PUBLIC "QT_NAMESPACE=${QT_NAMESPACE}")
endif()

qt_generate_qconfig_cpp()

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

if(NOT BUILD_SHARED_LIBS)
    install(
      TARGETS Core_qobject
      EXPORT "${INSTALL_CMAKE_NAMESPACE}Targets"
    )
endif()

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

add_qt_simd_part(Core SIMD mips_dsp
    SOURCES
        ../gui/painting/qt_mips_asm_dsp_p.h
        tools/qstring_mips_dsp_asm.S
)



#### Keys ignored in scope 1:.:.:corelib.pro:<TRUE>:
# ANDROID_BUNDLED_JAR_DEPENDENCIES = "jar/QtAndroid.jar"
# ANDROID_LIB_DEPENDENCIES = "plugins/platforms/android/libqtforandroid.so"
# ANDROID_PERMISSIONS = "android.permission.INTERNET" "android.permission.WRITE_EXTERNAL_STORAGE"
# CMAKE_DISABLED_FEATURES = "$$joinQT_DISABLED_FEATURES,"$$escape_expand(\\n)    ""
# CMAKE_HOST_DATA_DIR = "$$cmakeRelativePath$$[QT_HOST_DATA/src],$$[QT_INSTALL_PREFIX]"
# CMAKE_INSTALL_DATA_DIR = "$$cmakeRelativePath$$[QT_HOST_DATA],$$[QT_INSTALL_PREFIX]"
# CONFIG = "exceptions" "qt_tracepoints" "$$MODULE_CONFIG" "simd" "optimize_full"
# HOST_BINS = "$$[QT_HOST_BINS]"
# INSTALLS = "ctest_qt5_module_files" "cmake_qt5_umbrella_module_files"
# MODULE = "core"
# MODULE_CONFIG = "moc" "resources"
# QMAKE_DSYM_DEBUG_SCRIPT = "$$PWD/debug_script.py"
# QMAKE_DYNAMIC_LIST_FILE = "$$PWD/QtCore.dynlist"
# QMAKE_PKGCONFIG_VARIABLES = "host_bins" "qt_conf"
# QMAKE_SUBSTITUTES = "ctest_macros_file" "cmake_umbrella_config_file" "cmake_umbrella_config_module_location" "cmake_umbrella_config_module_location_for_install" "cmake_umbrella_config_version_file" "cmake_extras_mkspec_dir" "cmake_extras_mkspec_dir_for_install"
# _LOADED = "qt_module" "cmake_functions"
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

#### Keys ignored in scope 3:.:.:corelib.pro:NOT QT_NAMESPACE_ISEMPTY:
# MODULE_DEFINES = "QT_NAMESPACE=$$QT_NAMESPACE"

extend_target(Core CONDITION MSVC AND (TEST_architecture_arch STREQUAL "i386")
    LINK_OPTIONS
        "/BASE:0x67000000"
)

#### Keys ignored in scope 5:.:.:corelib.pro:FREEBSD OR OPENBSD:
# QMAKE_LFLAGS_NOUNDEF = <EMPTY>

extend_target(Core CONDITION QT_FEATURE_animation
    SOURCES
        animation/qabstractanimation.cpp animation/qabstractanimation.h animation/qabstractanimation_p.h
        animation/qanimationgroup.cpp animation/qanimationgroup.h animation/qanimationgroup_p.h
        animation/qparallelanimationgroup.cpp animation/qparallelanimationgroup.h animation/qparallelanimationgroup_p.h
        animation/qpauseanimation.cpp animation/qpauseanimation.h
        animation/qpropertyanimation.cpp animation/qpropertyanimation.h animation/qpropertyanimation_p.h
        animation/qsequentialanimationgroup.cpp animation/qsequentialanimationgroup.h animation/qsequentialanimationgroup_p.h
        animation/qvariantanimation.cpp animation/qvariantanimation.h animation/qvariantanimation_p.h
)

extend_target(Core CONDITION WIN32
    SOURCES
        global/qoperatingsystemversion_win.cpp global/qoperatingsystemversion_win_p.h
        io/qfilesystemengine_win.cpp
        io/qfsfileengine_win.cpp
        io/qlockfile_win.cpp
        kernel/qcoreapplication_win.cpp
        kernel/qelapsedtimer_win.cpp
        kernel/qsharedmemory_win.cpp
        kernel/qsystemsemaphore_win.cpp
        kernel/qwineventnotifier.cpp kernel/qwineventnotifier.h kernel/qwineventnotifier_p.h
        plugin/qsystemlibrary.cpp plugin/qsystemlibrary_p.h
        thread/qatomic_msvc.h
        thread/qthread_win.cpp
    # DEFINES # special case: remove
    LIBRARIES
        ws2_32
)

extend_target(Core CONDITION WIN32 AND NOT WINRT
    SOURCES
        io/qstandardpaths_win.cpp
        io/qstorageinfo_win.cpp
        io/qwindowspipereader.cpp io/qwindowspipereader_p.h
        io/qwindowspipewriter.cpp io/qwindowspipewriter_p.h
        kernel/qeventdispatcher_win.cpp kernel/qeventdispatcher_win_p.h
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
    PUBLIC_LIBRARIES
        mpr
        userenv
)

extend_target(Core CONDITION APPLE
    SOURCES
        global/qoperatingsystemversion_darwin.mm
    LIBRARIES
        ${FWCoreFoundation}
        ${FWFoundation}
)

extend_target(Core CONDITION APPLE_OSX
    SOURCES
        io/qstandardpaths_mac.mm
        io/qstorageinfo_mac.cpp
        kernel/qcfsocketnotifier.cpp kernel/qcfsocketnotifier_p.h
        kernel/qcore_foundation.mm
        kernel/qcore_mac.cpp kernel/qcore_mac_p.h
        kernel/qcore_mac_objc.mm
        kernel/qcoreapplication_mac.cpp
        kernel/qeventdispatcher_cf.mm kernel/qeventdispatcher_cf_p.h
    LIBRARIES
        ${FWAppKit}
        ${FWApplicationServices}
        ${FWCoreServices}
        ${FWFoundation}
        ${FWSecurity}
    PUBLIC_LIBRARIES
        ${FWDiskArbitration}
        ${FWIOKit}
)

extend_target(Core CONDITION INTEGRITY
    LIBRARIES
        ivfs
        net
        posix
        shm_client
        socket
    COMPILE_OPTIONS
        --pending_instantiations=128
)

#### Keys ignored in scope 13:.:.:corelib.pro:pathIsAbsolute(_ss_CMAKE_HOST_DATA_DIR):
# CMAKE_HOST_DATA_DIR = "$$[QT_HOST_DATA/src]/"
# CMAKE_HOST_DATA_DIR_IS_ABSOLUTE = "True"

#### Keys ignored in scope 14:.:.:corelib.pro:pathIsAbsolute(_ss_CMAKE_INSTALL_DATA_DIR):
# CMAKE_INSTALL_DATA_DIR = "$$[QT_HOST_DATA]/"
# CMAKE_INSTALL_DATA_DIR_IS_ABSOLUTE = "True"

#### Keys ignored in scope 17:.:global:global/global.pri:QT_CPU_FEATURES__ss_QT_ARCH___contains___f16c:
# f16c_cxx = "true"

#### Keys ignored in scope 19:.:global:global/global.pri:CLANG OR intel_icl OR ICC:
# f16c_cxx = "false"

#### Keys ignored in scope 21:.:global:global/global.pri:GCC AND f16c AND x86SimdAlways:
# f16c_cxx = "true"

#### Keys ignored in scope 23:.:global:global/global.pri:MSVC AND QT_CPU_FEATURES__ss_QT_ARCH___contains___avx:
# f16c_cxx = "true"

#### Keys ignored in scope 24:.:global:global/global.pri:else:
# f16c_cxx = "false"

# extend_target(Core CONDITION _ss_f16c_cxx ... # special case: Handled manually:

#### Keys ignored in scope 26:.:global:global/global.pri:else:
# F16C_SOURCES = "global/qfloat16_f16c.c"

extend_target(Core CONDITION linux_x_ OR hurd_x_ AND NOT cross_compile AND NOT static AND NOT _x_-armcc_x_
    DEFINES
        ELF_INTERPRETER=\\\"=Creadelf-l/bin/ls|perl-n-e\'/programinterpreter:.*]/{print$1;}\'\\\"
    LINK_OPTIONS
        "-Wl,-e,qt_core_boilerplate"
)

extend_target(Core CONDITION LINUX AND NOT static
    SOURCES
        global/minimum-linux_p.h
)

#### Keys ignored in scope 31:.:global:global/global.pri:precompile_header:
# NO_PCH_ASM = "global/minimum-linux.S"
# QMAKE_EXTRA_COMPILERS = "no_pch_assembler"
# no_pch_assembler.commands = "$$QMAKE_CC" "-c" "$(CFLAGS)" "$(INCPATH)" "${QMAKE_FILE_IN}" "-o" "${QMAKE_FILE_OUT}"
# no_pch_assembler.dependency_type = "TYPE_C"
# no_pch_assembler.input = "NO_PCH_ASM"
# no_pch_assembler.name = "compiling[no_pch]" "${QMAKE_FILE_IN}"
# no_pch_assembler.output = "${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${firstQMAKE_EXT_OBJ}"

#### Keys ignored in scope 32:.:global:global/global.pri:silent:
# no_pch_assembler.commands = "@echo" "compiling[no_pch]" "${QMAKE_FILE_IN}" "&&"

extend_target(Core CONDITION LINUX AND NOT precompile_header AND NOT static
    SOURCES
        global/minimum-linux.S
)

extend_target(Core CONDITION QT_FEATURE_slog2
    LIBRARIES
        Slog2::Slog2
)

extend_target(Core CONDITION QT_FEATURE_journald
    LIBRARIES
        PkgConfig::Libsystemd
)

#### Keys ignored in scope 36:.:global:global/global.pri:GCC AND ltcg:
# QMAKE_EXTRA_COMPILERS = "versiontagging_compiler"
# versiontagging_compiler.commands = "$$QMAKE_CXX" "-c" "$(CXXFLAGS)" "$(INCPATH)" "-fno-lto" "-o" "${QMAKE_FILE_OUT}" "${QMAKE_FILE_IN}"
# versiontagging_compiler.dependency_type = "TYPE_C"
# versiontagging_compiler.input = "VERSIONTAGGING_SOURCES"
# versiontagging_compiler.name = "compiling[versiontagging]" "${QMAKE_FILE_IN}"
# versiontagging_compiler.output = "${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${firstQMAKE_EXT_OBJ}"
# versiontagging_compiler.variable_out = "OBJECTS"

#### Keys ignored in scope 37:.:global:global/global.pri:silent:
# versiontagging_compiler.commands = "@echo" "compiling[versiontagging]" "${QMAKE_FILE_IN}" "&&"

extend_target(Core CONDITION UNIX
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

extend_target(Core CONDITION QT_FEATURE_thread
    SOURCES
        thread/qatomic.cpp thread/qatomic.h
        thread/qatomic_bootstrap.h
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
        thread/qmutex_win.cpp
        thread/qwaitcondition_win.cpp
)

extend_target(Core CONDITION QT_FEATURE_thread AND UNIX
    SOURCES
        thread/qwaitcondition_unix.cpp
)

extend_target(Core CONDITION APPLE AND QT_FEATURE_thread
    SOURCES
        thread/qmutex_mac.cpp
)

extend_target(Core CONDITION LINUX AND QT_FEATURE_thread
    SOURCES
        thread/qmutex_linux.cpp
)

extend_target(Core CONDITION QT_FEATURE_thread AND UNIX AND NOT APPLE AND NOT LINUX
    SOURCES
        thread/qmutex_unix.cpp
)

extend_target(Core CONDITION QT_FEATURE_future
    SOURCES
        thread/qexception.cpp thread/qexception.h
        thread/qfuture.h
        thread/qfutureinterface.cpp thread/qfutureinterface.h thread/qfutureinterface_p.h
        thread/qfuturesynchronizer.h
        thread/qfuturewatcher.cpp thread/qfuturewatcher.h thread/qfuturewatcher_p.h
        thread/qresultstore.cpp thread/qresultstore.h
)

extend_target(Core CONDITION QT_FEATURE_std_atomic64
    PUBLIC_LIBRARIES
        Atomic
)

extend_target(Core CONDITION ICC
    COMPILE_OPTIONS
        -fp-model
        strict
)

extend_target(Core CONDITION MSVC
    SOURCES
        tools/qvector_msvc.cpp
)

extend_target(Core CONDITION APPLE_OSX AND NOT NACL
    SOURCES
        kernel/qelapsedtimer_mac.cpp
        tools/qlocale_mac.mm
)

extend_target(Core CONDITION UNIX AND (NACL OR NOT APPLE_OSX)
    SOURCES
        tools/qlocale_unix.cpp
)

extend_target(Core CONDITION WIN32 AND (NACL OR NOT APPLE_OSX)
    SOURCES
        tools/qlocale_win.cpp
)

extend_target(Core CONDITION QT_FEATURE_icu
    SOURCES
        tools/qcollator_icu.cpp
        tools/qlocale_icu.cpp
    LIBRARIES
        ICU::i18n ICU::uc ICU::data
)

extend_target(Core CONDITION WIN32 AND NOT QT_FEATURE_icu
    SOURCES
        tools/qcollator_win.cpp
)

extend_target(Core CONDITION APPLE_OSX AND NOT QT_FEATURE_icu
    SOURCES
        tools/qcollator_macx.cpp
)

extend_target(Core CONDITION UNIX AND NOT APPLE_OSX AND NOT QT_FEATURE_icu
    SOURCES
        tools/qcollator_posix.cpp
)

extend_target(Core CONDITION QT_FEATURE_timezone
    SOURCES
        tools/qtimezone.cpp tools/qtimezone.h
        tools/qtimezoneprivate.cpp tools/qtimezoneprivate_p.h
        tools/qtimezoneprivate_data_p.h
)

extend_target(Core CONDITION APPLE AND QT_FEATURE_timezone AND NOT NACL
    SOURCES
        tools/qtimezoneprivate_mac.mm
)

extend_target(Core CONDITION ANDROID AND QT_FEATURE_timezone AND NOT ANDROID_EMBEDDED AND (NACL OR NOT APPLE)
    SOURCES
        tools/qtimezoneprivate_android.cpp
)

extend_target(Core CONDITION QT_FEATURE_timezone AND UNIX AND (ANDROID_EMBEDDED OR NOT ANDROID) AND (NACL OR NOT APPLE)
    SOURCES
        tools/qtimezoneprivate_tz.cpp
)

extend_target(Core CONDITION QT_FEATURE_icu AND QT_FEATURE_timezone AND UNIX AND (ANDROID_EMBEDDED OR NOT ANDROID) AND (NACL OR NOT APPLE)
    SOURCES
        tools/qtimezoneprivate_icu.cpp
)

extend_target(Core CONDITION QT_FEATURE_icu AND QT_FEATURE_timezone AND WIN32 AND (ANDROID_EMBEDDED OR NOT ANDROID) AND (NACL OR NOT APPLE)
    SOURCES
        tools/qtimezoneprivate_icu.cpp
)

extend_target(Core CONDITION QT_FEATURE_timezone AND WIN32 AND NOT QT_FEATURE_icu AND (ANDROID_EMBEDDED OR NOT ANDROID) AND (NACL OR NOT APPLE)
    SOURCES
        tools/qtimezoneprivate_win.cpp
)

extend_target(Core CONDITION QT_FEATURE_datetimeparser
    SOURCES
        tools/qdatetimeparser.cpp tools/qdatetimeparser_p.h
)

extend_target(Core CONDITION QT_FEATURE_regularexpression
    SOURCES
        tools/qregularexpression.cpp tools/qregularexpression.h
    LIBRARIES
        PCRE2
)

extend_target(Core CONDITION QT_FEATURE_commandlineparser
    SOURCES
        tools/qcommandlineoption.cpp tools/qcommandlineoption.h
        tools/qcommandlineparser.cpp tools/qcommandlineparser.h
)

extend_target(Core CONDITION UNIX AND NOT HAIKU AND NOT INTEGRITY AND NOT VXWORKS AND NOT WASM AND (NOT APPLE_OSX OR NOT ICC)
    LIBRARIES
        m
)

extend_target(Core CONDITION QT_FEATURE_zstd
    LIBRARIES
        ZSTD::ZSTD
)

extend_target(Core CONDITION QT_FEATURE_filesystemwatcher
    SOURCES
        io/qfilesystemwatcher.cpp io/qfilesystemwatcher.h io/qfilesystemwatcher_p.h
        io/qfilesystemwatcher_polling.cpp io/qfilesystemwatcher_polling_p.h
)

extend_target(Core CONDITION QT_FEATURE_filesystemwatcher AND WIN32
    SOURCES
        io/qfilesystemwatcher_win.cpp io/qfilesystemwatcher_win_p.h
)

extend_target(Core CONDITION APPLE_OSX AND QT_FEATURE_filesystemwatcher
    SOURCES
        io/qfilesystemwatcher_fsevents.mm io/qfilesystemwatcher_fsevents_p.h
)

extend_target(Core CONDITION QT_FEATURE_filesystemwatcher AND QT_FEATURE_inotify AND UNIX AND NOT APPLE_OSX
    SOURCES
        io/qfilesystemwatcher_inotify.cpp io/qfilesystemwatcher_inotify_p.h
)

extend_target(Core CONDITION QT_FEATURE_filesystemwatcher AND UNIX AND NOT APPLE_OSX AND NOT QT_FEATURE_inotify AND (APPLE OR FREEBSD OR NETBSD OR OPENBSD)
    SOURCES
        io/qfilesystemwatcher_kqueue.cpp io/qfilesystemwatcher_kqueue_p.h
)

extend_target(Core CONDITION QT_FEATURE_processenvironment
    SOURCES
        io/qprocess.cpp io/qprocess.h io/qprocess_p.h
)

extend_target(Core CONDITION QT_FEATURE_processenvironment AND WIN32 AND NOT WINRT
    SOURCES
        io/qprocess_win.cpp
)

extend_target(Core CONDITION QT_FEATURE_processenvironment AND UNIX
    SOURCES
        io/qprocess_unix.cpp
)

extend_target(Core CONDITION QT_FEATURE_settings
    SOURCES
        io/qsettings.cpp io/qsettings.h io/qsettings_p.h
)

extend_target(Core CONDITION QT_FEATURE_settings AND WIN32 AND NOT WINRT
    SOURCES
        io/qsettings_win.cpp
)

extend_target(Core CONDITION QT_FEATURE_settings AND WINRT
    SOURCES
        io/qsettings_winrt.cpp
)

extend_target(Core CONDITION APPLE AND QT_FEATURE_settings AND NOT NACL
    SOURCES
        io/qsettings_mac.cpp
)

extend_target(Core CONDITION QT_FEATURE_filesystemiterator AND WIN32
    SOURCES
        io/qfilesystemiterator_win.cpp
)

extend_target(Core CONDITION WINRT
    SOURCES
        io/qstandardpaths_winrt.cpp
        io/qstorageinfo_stub.cpp
        kernel/qeventdispatcher_winrt.cpp kernel/qeventdispatcher_winrt_p.h
        kernel/qfunctions_fake_env_p.h
        kernel/qfunctions_winrt.cpp kernel/qfunctions_winrt.h
)

extend_target(Core CONDITION UNIX AND NOT APPLE_UIKIT AND NOT INTEGRITY
    SOURCES
        ../3rdparty/forkfd/forkfd.h
        io/forkfd_qt.cpp
    INCLUDE_DIRECTORIES
        ../3rdparty/forkfd
)

extend_target(Core CONDITION APPLE_OSX AND QT_FEATURE_processenvironment
    SOURCES
        io/qprocess_darwin.mm
)

extend_target(Core CONDITION ANDROID AND NOT ANDROID_EMBEDDED
    SOURCES
        io/qstandardpaths_android.cpp
        io/qstorageinfo_unix.cpp
        kernel/qjni.cpp kernel/qjni_p.h
        kernel/qjnihelpers.cpp kernel/qjnihelpers_p.h
        kernel/qjnionload.cpp
)

extend_target(Core CONDITION HAIKU AND (ANDROID_EMBEDDED OR NOT ANDROID)
    SOURCES
        io/qstandardpaths_haiku.cpp
        io/qstorageinfo_unix.cpp
    PUBLIC_LIBRARIES
        be
)

extend_target(Core CONDITION UNIX AND NOT APPLE_OSX AND NOT HAIKU AND (ANDROID_EMBEDDED OR NOT ANDROID)
    SOURCES
        io/qstandardpaths_unix.cpp
        io/qstorageinfo_unix.cpp
)

extend_target(Core CONDITION QT_FEATURE_proxymodel
    SOURCES
        itemmodels/qabstractproxymodel.cpp itemmodels/qabstractproxymodel.h itemmodels/qabstractproxymodel_p.h
)

extend_target(Core CONDITION QT_FEATURE_concatenatetablesproxymodel AND QT_FEATURE_proxymodel
    SOURCES
        itemmodels/qconcatenatetablesproxymodel.cpp itemmodels/qconcatenatetablesproxymodel.h
)

extend_target(Core CONDITION QT_FEATURE_identityproxymodel AND QT_FEATURE_proxymodel
    SOURCES
        itemmodels/qidentityproxymodel.cpp itemmodels/qidentityproxymodel.h
)

extend_target(Core CONDITION QT_FEATURE_proxymodel AND QT_FEATURE_sortfilterproxymodel
    SOURCES
        itemmodels/qsortfilterproxymodel.cpp itemmodels/qsortfilterproxymodel.h
)

extend_target(Core CONDITION QT_FEATURE_proxymodel AND QT_FEATURE_transposeproxymodel
    SOURCES
        itemmodels/qtransposeproxymodel.cpp itemmodels/qtransposeproxymodel.h itemmodels/qtransposeproxymodel_p.h
)

extend_target(Core CONDITION QT_FEATURE_stringlistmodel
    SOURCES
        itemmodels/qstringlistmodel.cpp itemmodels/qstringlistmodel.h
)

extend_target(Core CONDITION QT_FEATURE_library
    SOURCES
        plugin/qelfparser_p.cpp plugin/qelfparser_p.h
        plugin/qlibrary.cpp plugin/qlibrary.h plugin/qlibrary_p.h
        plugin/qmachparser.cpp plugin/qmachparser_p.h
)

extend_target(Core CONDITION QT_FEATURE_library AND UNIX
    SOURCES
        plugin/qlibrary_unix.cpp
)

extend_target(Core CONDITION QT_FEATURE_library AND WIN32
    SOURCES
        plugin/qlibrary_win.cpp
)

extend_target(Core CONDITION QT_FEATURE_dlopen AND QT_FEATURE_library
    LIBRARIES
        ${CMAKE_DL_LIBS}
)

extend_target(Core CONDITION APPLE_OSX AND (APPLE_IOS OR APPLE_TVOS)
    LIBRARIES
        ${FWUIKit}
)

extend_target(Core CONDITION APPLE_OSX AND APPLE_WATCHOS
    LIBRARIES
        ${FWWatchKit}
)

extend_target(Core CONDITION NACL
    SOURCES
        kernel/qfunctions_nacl.cpp kernel/qfunctions_nacl.h
)

extend_target(Core CONDITION INTEGRITY OR (NACL AND UNIX) OR (UNIX AND NOT APPLE)
    SOURCES
        kernel/qelapsedtimer_unix.cpp
)

extend_target(Core CONDITION QT_FEATURE_poll_select AND UNIX
    SOURCES
        kernel/qpoll.cpp
)

extend_target(Core CONDITION QT_FEATURE_glib AND UNIX
    SOURCES
        kernel/qeventdispatcher_glib.cpp kernel/qeventdispatcher_glib_p.h
    LIBRARIES
        GLIB2::GLIB2
)

extend_target(Core CONDITION QT_FEATURE_clock_gettime AND UNIX
    LIBRARIES
        WrapRt
)

extend_target(Core CONDITION UNIX AND NOT ANDROID
    SOURCES
        kernel/qsharedmemory_posix.cpp
        kernel/qsharedmemory_systemv.cpp
        kernel/qsharedmemory_unix.cpp
        kernel/qsystemsemaphore_posix.cpp
        kernel/qsystemsemaphore_systemv.cpp
        kernel/qsystemsemaphore_unix.cpp
)

extend_target(Core CONDITION ANDROID
    SOURCES
        kernel/qsharedmemory_android.cpp
        kernel/qsystemsemaphore_android.cpp
)

extend_target(Core CONDITION VXWORKS
    SOURCES
        kernel/qfunctions_vxworks.cpp kernel/qfunctions_vxworks.h
)

extend_target(Core CONDITION QNX AND QT_FEATURE_qqnx_pps
    SOURCES
        kernel/qppsattribute.cpp kernel/qppsattribute_p.h
        kernel/qppsattributeprivate_p.h
        kernel/qppsobject.cpp kernel/qppsobject_p.h
        kernel/qppsobjectprivate_p.h
    LIBRARIES
        PPS::PPS
)

extend_target(Core CONDITION QT_FEATURE_textcodec
    SOURCES
        codecs/qlatincodec.cpp codecs/qlatincodec_p.h
        codecs/qsimplecodec.cpp codecs/qsimplecodec_p.h
        codecs/qtextcodec.cpp codecs/qtextcodec.h
)

extend_target(Core CONDITION QT_FEATURE_codecs AND QT_FEATURE_textcodec
    SOURCES
        codecs/qisciicodec.cpp codecs/qisciicodec_p.h
        codecs/qtsciicodec.cpp codecs/qtsciicodec_p.h
)

extend_target(Core CONDITION QT_FEATURE_icu AND QT_FEATURE_textcodec
    SOURCES
        codecs/qicucodec.cpp codecs/qicucodec_p.h
)

extend_target(Core CONDITION QT_FEATURE_big_codecs AND QT_FEATURE_textcodec AND NOT QT_FEATURE_icu
    SOURCES
        codecs/qbig5codec.cpp codecs/qbig5codec_p.h
        codecs/qeucjpcodec.cpp codecs/qeucjpcodec_p.h
        codecs/qeuckrcodec.cpp codecs/qeuckrcodec_p.h
        codecs/qgb18030codec.cpp codecs/qgb18030codec_p.h
        codecs/qjiscodec.cpp codecs/qjiscodec_p.h
        codecs/qjpunicode.cpp
        codecs/qsjiscodec.cpp codecs/qsjiscodec_p.h
)

extend_target(Core CONDITION QT_FEATURE_iconv AND QT_FEATURE_textcodec AND NOT QT_FEATURE_icu
    SOURCES
        codecs/qiconvcodec.cpp codecs/qiconvcodec_p.h
    LIBRARIES
        iconv
)

extend_target(Core CONDITION QT_FEATURE_textcodec AND WIN32 AND NOT QT_FEATURE_icu
    SOURCES
        codecs/qwindowscodec.cpp codecs/qwindowscodec_p.h
)

extend_target(Core CONDITION QT_FEATURE_qeventtransition
    SOURCES
        statemachine/qeventtransition.cpp statemachine/qeventtransition.h statemachine/qeventtransition_p.h
)

extend_target(Core CONDITION QT_FEATURE_mimetype
    SOURCES
        mimetypes/qmimedatabase.cpp mimetypes/qmimedatabase.h mimetypes/qmimedatabase_p.h
        mimetypes/qmimeglobpattern.cpp mimetypes/qmimeglobpattern_p.h
        mimetypes/qmimemagicrule.cpp mimetypes/qmimemagicrule_p.h
        mimetypes/qmimemagicrulematcher.cpp mimetypes/qmimemagicrulematcher_p.h
        mimetypes/qmimeprovider.cpp mimetypes/qmimeprovider_p.h
        mimetypes/qmimetype.cpp mimetypes/qmimetype.h mimetypes/qmimetype_p.h
        mimetypes/qmimetypeparser.cpp mimetypes/qmimetypeparser_p.h
)

# Resources:
set_source_files_properties("mimetypes/mime/packages/freedesktop.org.xml"
    PROPERTIES alias "freedesktop.org.xml")
add_qt_resource(Core "mimetypes" PREFIX "/qt-project.org/qmime/packages" BASE "mimetypes" FILES
    mime/packages/freedesktop.org.xml)


extend_target(Core CONDITION WASM
    SOURCES
        platform/wasm/qstdweb.cpp platform/wasm/qstdweb_p.h
)

# special case begin
# needs refinement for the other compiler specific cases.
if(TEST_arch_${TEST_architecture_arch}_subarch_f16c)
    # Equivalent to contains(QT_CPU_FEATURES.$$QT_ARCH, f16c)
    extend_target(Core
        DEFINES
            QFLOAT16_INCLUDE_FAST
    )
else()
    add_qt_simd_part(Core SIMD f16c
        SOURCES
            global/qfloat16_f16c.c
    )
endif()


add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/global/qfloat16tables.cpp"
                   COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::qfloat16-tables
                   ARGS "${CMAKE_CURRENT_BINARY_DIR}/global/qfloat16tables.cpp"
                   DEPENDS global/qfloat16.h
                   COMMENT qfloat16-tables
                   VERBATIM)
target_sources(Core PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/global/qfloat16tables.cpp")


set_source_files_properties(
    thread/qmutex_linux.cpp
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

qt_internal_add_link_flags_gc_sections(Core PRIVATE)
# special case end

qt_create_tracepoints(Core qtcore.tracepoints)
add_qt_docs(
    doc/qtcore.qdocconf
)


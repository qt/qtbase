TARGET	   = QtCore
QT         =
CONFIG    += exceptions

MODULE = core     # not corelib, as per project file
MODULE_CONFIG = moc resources
!isEmpty(QT_NAMESPACE): MODULE_DEFINES = QT_NAMESPACE=$$QT_NAMESPACE

TRACEPOINT_PROVIDER = $$PWD/qtcore.tracepoints
CONFIG += qt_tracepoints

CONFIG += $$MODULE_CONFIG
DEFINES += $$MODULE_DEFINES
DEFINES += QT_NO_USING_NAMESPACE QT_NO_FOREACH
msvc:equals(QT_ARCH, i386): QMAKE_LFLAGS += /BASE:0x67000000

CONFIG += simd optimize_full

QMAKE_DOCS = $$PWD/doc/qtcore.qdocconf

ANDROID_LIB_DEPENDENCIES = \
    plugins/platforms/android/libqtforandroid.so
ANDROID_BUNDLED_JAR_DEPENDENCIES = \
    jar/QtAndroid.jar
ANDROID_PERMISSIONS = \
    android.permission.INTERNET \
    android.permission.WRITE_EXTERNAL_STORAGE

# QtCore can't be compiled with -Wl,-no-undefined because it uses the "environ"
# variable and on FreeBSD and OpenBSD, this variable is in the final executable itself.
# OpenBSD 6.0 will include environ in libc.
freebsd|openbsd: QMAKE_LFLAGS_NOUNDEF =

qtConfig(animation): include(animation/animation.pri)
include(global/global.pri)
include(thread/thread.pri)
include(tools/tools.pri)
include(io/io.pri)
include(itemmodels/itemmodels.pri)
include(plugin/plugin.pri)
include(kernel/kernel.pri)
include(codecs/codecs.pri)
include(serialization/serialization.pri)
include(statemachine/statemachine.pri)
include(mimetypes/mimetypes.pri)

win32 {
    LIBS_PRIVATE += -lws2_32
    !winrt {
        LIBS_PRIVATE += -lkernel32 -luser32 -lshell32 -luuid -lole32 -ladvapi32 -lwinmm
    }
}

darwin {
    osx {
        LIBS_PRIVATE += -framework ApplicationServices
        LIBS_PRIVATE += -framework CoreServices
    }
    LIBS_PRIVATE += -framework CoreFoundation
    LIBS_PRIVATE += -framework Foundation
}

integrity {
    LIBS_PRIVATE += -lposix -livfs -lsocket -lnet -lshm_client
}

QMAKE_DYNAMIC_LIST_FILE = $$PWD/QtCore.dynlist

contains(DEFINES,QT_EVAL):include(eval.pri)

HOST_BINS = $$[QT_HOST_BINS]
host_bins.name = host_bins
host_bins.variable = HOST_BINS

qt_conf.name = qt_config
qt_conf.variable = QT_CONFIG

QMAKE_PKGCONFIG_VARIABLES += host_bins qt_conf

load(qt_module)

# Override qt_module, so the symbols are actually included into the library.
win32: DEFINES -= QT_NO_CAST_TO_ASCII

ctest_macros_file.input = $$PWD/Qt5CTestMacros.cmake
ctest_macros_file.output = $$DESTDIR/cmake/Qt5Core/Qt5CTestMacros.cmake
ctest_macros_file.CONFIG = verbatim

cmake_umbrella_config_file.input = $$PWD/Qt5Config.cmake.in
cmake_umbrella_config_file.output = $$DESTDIR/cmake/Qt5/Qt5Config.cmake

cmake_umbrella_config_module_location.input = $$PWD/Qt5ModuleLocation.cmake.in
cmake_umbrella_config_module_location.output = $$DESTDIR/cmake/Qt5/Qt5ModuleLocation.cmake

cmake_umbrella_config_module_location_for_install.input = $$PWD/Qt5ModuleLocationForInstall.cmake.in
cmake_umbrella_config_module_location_for_install.output = $$DESTDIR/cmake/install/Qt5/Qt5ModuleLocation.cmake

cmake_umbrella_config_version_file.input = $$PWD/../../mkspecs/features/data/cmake/Qt5ConfigVersion.cmake.in
cmake_umbrella_config_version_file.output = $$DESTDIR/cmake/Qt5/Qt5ConfigVersion.cmake

load(cmake_functions)

defineTest(pathIsAbsolute) {
    p = $$clean_path($$1)
    !isEmpty(p):isEqual(p, $$absolute_path($$p)): return(true)
    return(false)
}

##### This requires fixing, so that the feature system works with cmake as well
CMAKE_DISABLED_FEATURES = $$join(QT_DISABLED_FEATURES, "$$escape_expand(\\n)    ")

CMAKE_HOST_DATA_DIR = $$cmakeRelativePath($$[QT_HOST_DATA/src], $$[QT_INSTALL_PREFIX])
pathIsAbsolute($$CMAKE_HOST_DATA_DIR) {
    CMAKE_HOST_DATA_DIR = $$[QT_HOST_DATA/src]/
    CMAKE_HOST_DATA_DIR_IS_ABSOLUTE = True
}

cmake_extras_mkspec_dir.input = $$PWD/Qt5CoreConfigExtrasMkspecDir.cmake.in
cmake_extras_mkspec_dir.output = $$DESTDIR/cmake/Qt5Core/Qt5CoreConfigExtrasMkspecDir.cmake

CMAKE_INSTALL_DATA_DIR = $$cmakeRelativePath($$[QT_HOST_DATA], $$[QT_INSTALL_PREFIX])
pathIsAbsolute($$CMAKE_INSTALL_DATA_DIR) {
    CMAKE_INSTALL_DATA_DIR = $$[QT_HOST_DATA]/
    CMAKE_INSTALL_DATA_DIR_IS_ABSOLUTE = True
}

cmake_extras_mkspec_dir_for_install.input = $$PWD/Qt5CoreConfigExtrasMkspecDirForInstall.cmake.in
cmake_extras_mkspec_dir_for_install.output = $$DESTDIR/cmake/install/Qt5Core/Qt5CoreConfigExtrasMkspecDir.cmake

cmake_qt5_umbrella_module_files.files = \
    $$cmake_umbrella_config_file.output \
    $$cmake_umbrella_config_version_file.output \
    $$cmake_umbrella_config_module_location_for_install.output

cmake_qt5_umbrella_module_files.path = $$[QT_INSTALL_LIBS]/cmake/Qt5

QMAKE_SUBSTITUTES += \
    ctest_macros_file \
    cmake_umbrella_config_file \
    cmake_umbrella_config_module_location \
    cmake_umbrella_config_module_location_for_install \
    cmake_umbrella_config_version_file \
    cmake_extras_mkspec_dir \
    cmake_extras_mkspec_dir_for_install

ctest_qt5_module_files.files += $$ctest_macros_file.output $$cmake_extras_mkspec_dir_for_install.output

ctest_qt5_module_files.path = $$[QT_INSTALL_LIBS]/cmake/Qt5Core

INSTALLS += ctest_qt5_module_files cmake_qt5_umbrella_module_files

QMAKE_DSYM_DEBUG_SCRIPT = $$PWD/debug_script.py

TARGET	   = QtCore
QT         =
CONFIG    += exceptions

MODULE = core     # not corelib, as per project file
MODULE_CONFIG = moc resources
!isEmpty(QT_NAMESPACE): MODULE_DEFINES = QT_NAMESPACE=$$QT_NAMESPACE

CONFIG += $$MODULE_CONFIG
DEFINES   += QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x67000000
irix-cc*:QMAKE_CXXFLAGS += -no_prelink -ptused

# otherwise mingw headers do not declare common functions like putenv
win32-g++*:QMAKE_CXXFLAGS_CXX11 = -std=gnu++0x

QMAKE_DOCS = $$PWD/doc/qtcore.qdocconf

ANDROID_JAR_DEPENDENCIES = \
    jar/QtAndroid.jar
ANDROID_LIB_DEPENDENCIES = \
    plugins/platforms/android/libqtforandroid.so \
    libs/libgnustl_shared.so
ANDROID_BUNDLED_JAR_DEPENDENCIES = \
    jar/QtAndroid-bundled.jar

load(qt_module)

include(animation/animation.pri)
include(arch/arch.pri)
include(global/global.pri)
include(thread/thread.pri)
include(tools/tools.pri)
include(io/io.pri)
include(itemmodels/itemmodels.pri)
include(json/json.pri)
include(plugin/plugin.pri)
include(kernel/kernel.pri)
include(codecs/codecs.pri)
include(statemachine/statemachine.pri)
include(mimetypes/mimetypes.pri)
include(xml/xml.pri)

mac|darwin {
    !ios {
        LIBS_PRIVATE += -framework ApplicationServices
        LIBS_PRIVATE += -framework CoreServices
        LIBS_PRIVATE += -framework Foundation
    }
    LIBS_PRIVATE += -framework CoreFoundation
}
win32:DEFINES-=QT_NO_CAST_TO_ASCII
DEFINES += $$MODULE_DEFINES

QMAKE_LIBS += $$QMAKE_LIBS_CORE

QMAKE_DYNAMIC_LIST_FILE = $$PWD/QtCore.dynlist

contains(DEFINES,QT_EVAL):include(eval.pri)

HOST_BINS = $$[QT_HOST_BINS]
host_bins.name = host_bins
host_bins.variable = HOST_BINS

qt_conf.name = qt_config
qt_conf.variable = QT_CONFIG

QMAKE_PKGCONFIG_VARIABLES += host_bins qt_conf

ctest_macros_file.input = $$PWD/Qt5CTestMacros.cmake
ctest_macros_file.output = $$DESTDIR/cmake/Qt5Core/Qt5CTestMacros.cmake
ctest_macros_file.CONFIG = verbatim

cmake_umbrella_config_file.input = $$PWD/Qt5Config.cmake.in
cmake_umbrella_config_file.output = $$DESTDIR/cmake/Qt5/Qt5Config.cmake

cmake_umbrella_config_version_file.input = $$PWD/../../mkspecs/features/data/cmake/Qt5ConfigVersion.cmake.in
cmake_umbrella_config_version_file.output = $$DESTDIR/cmake/Qt5/Qt5ConfigVersion.cmake

cmake_qt5_umbrella_module_files.files = $$cmake_umbrella_config_file.output $$cmake_umbrella_config_version_file.output
cmake_qt5_umbrella_module_files.path = $$[QT_INSTALL_LIBS]/cmake/Qt5

QMAKE_SUBSTITUTES += ctest_macros_file cmake_umbrella_config_file cmake_umbrella_config_version_file

ctest_qt5_module_files.files += $$ctest_macros_file.output

ctest_qt5_module_files.path = $$[QT_INSTALL_LIBS]/cmake/Qt5Core

INSTALLS += ctest_qt5_module_files cmake_qt5_umbrella_module_files

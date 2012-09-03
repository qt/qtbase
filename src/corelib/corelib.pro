TARGET	   = QtCore
QT         =
CONFIG    += exceptions

MODULE = core     # not corelib, as per project file
MODULE_CONFIG = moc resources

DEFINES   += QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x67000000
irix-cc*:QMAKE_CXXFLAGS += -no_prelink -ptused

# otherwise mingw headers do not declare common functions like putenv
win32-g++*:QMAKE_CXXFLAGS_CXX11 = -std=gnu++0x

QMAKE_DOCS = $$PWD/doc/qtcore.qdocconf

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
mac:lib_bundle:DEFINES += QT_NO_DEBUG_PLUGIN_CHECK
win32:DEFINES-=QT_NO_CAST_TO_ASCII

QMAKE_LIBS += $$QMAKE_LIBS_CORE

QMAKE_DYNAMIC_LIST_FILE = $$PWD/QtCore.dynlist

contains(DEFINES,QT_EVAL):include(eval.pri)

load(moc)
load(resources)

moc_dir.name = moc_location
moc_dir.variable = QMAKE_MOC

rcc_dir.name = rcc_location
rcc_dir.variable = QMAKE_RCC

QMAKE_PKGCONFIG_VARIABLES += moc_dir rcc_dir

# These are aliens, but Linguist installs no own module, and it fits here best.

qtPrepareTool(QMAKE_LUPDATE, lupdate)
qtPrepareTool(QMAKE_LRELEASE, lrelease)

lupdate_dir.name = lupdate_location
lupdate_dir.variable = QMAKE_LUPDATE

lrelease_dir.name = lrelease_location
lrelease_dir.variable = QMAKE_LRELEASE

QMAKE_PKGCONFIG_VARIABLES += lupdate_dir lrelease_dir

ctest_macros_file.input = $$PWD/Qt5CTestMacros.cmake
ctest_macros_file.output = $$DESTDIR/cmake/Qt5Core/Qt5CTestMacros.cmake
ctest_macros_file.CONFIG = verbatim

QMAKE_SUBSTITUTES += ctest_macros_file

ctest_qt5_module_files.files += $$ctest_macros_file.output

ctest_qt5_module_files.path = $$[QT_INSTALL_LIBS]/cmake/Qt5Core

INSTALLS += ctest_qt5_module_files

load(qt_module)

TARGET	   = QtCore
QPRO_PWD   = $$PWD
QT         =

CONFIG += module
MODULE = core     # not corelib, as per project file
MODULE_PRI = ../modules/qt_core.pri

DEFINES   += QT_BUILD_CORE_LIB QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x67000000
irix-cc*:QMAKE_CXXFLAGS += -no_prelink -ptused

load(qt_module_config)

HEADERS += $$QT_SOURCE_TREE/src/corelib/qtcoreversion.h

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
    contains(QT_CONFIG, coreservices) {
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

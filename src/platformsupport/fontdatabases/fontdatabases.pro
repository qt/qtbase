TARGET = QtFontDatabaseSupport
MODULE = fontdatabase_support

QT = core-private gui-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../../corelib/global/qt_pch.h

unix {
    include($$PWD/genericunix/genericunix.pri)
}

qtConfig(fontconfig) {
    include($$PWD/fontconfig/fontconfig.pri)
}

win32 {
    include($$PWD/windows/windows.pri)
}

load(qt_module)

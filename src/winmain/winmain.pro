# Additional Qt project file for qtmain lib on Windows
!win32:error("$$_FILE_ is intended only for Windows!")

TEMPLATE = lib
TARGET = qtmain
DESTDIR = $$QT.core.libs

CONFIG += static
QT = core

DEFINES += QT_NO_FOREACH

qtConfig(debug_and_release): CONFIG += build_all

msvc: QMAKE_CFLAGS_DEBUG -= -Zi
msvc: QMAKE_CXXFLAGS_DEBUG -= -Zi
msvc: QMAKE_CFLAGS_DEBUG *= -Z7
msvc: QMAKE_CXXFLAGS_DEBUG *= -Z7
mingw: DEFINES += QT_NEEDS_QMAIN

winrt {
    SOURCES = qtmain_winrt.cpp
} else {
    CONFIG -= qt
    SOURCES = qtmain_win.cpp
    QMAKE_USE_PRIVATE += shell32
}

load(qt_installs)

TARGET = $$qtLibraryTarget($$TARGET$$QT_LIBINFIX) #do this towards the end

load(qt_targets)
load(qt_build_paths)
load(qt_common)

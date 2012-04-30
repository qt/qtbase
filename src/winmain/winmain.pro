# Additional Qt project file for qtmain lib on Windows
TEMPLATE = lib
TARGET	 = qtmain
QT       =

CONFIG	+= staticlib warn_on
CONFIG	-= qt shared

win32 {
    win32-msvc*:QMAKE_CFLAGS_DEBUG -= -Zi
    win32-msvc*:QMAKE_CXXFLAGS_DEBUG -= -Zi
    win32-msvc*:QMAKE_CFLAGS_DEBUG *= -Z7
    win32-msvc*:QMAKE_CXXFLAGS_DEBUG *= -Z7
    win32-g++*:DEFINES += QT_NEEDS_QMAIN
	win32-g++*:DEFINES += QT_NEEDS_QMAIN
	win32-borland:DEFINES += QT_NEEDS_QMAIN
	SOURCES		= qtmain_win.cpp
	CONFIG		+= png
	INCLUDEPATH	+= tmp $$QT.core.includes
}

!win32:error("$$_FILE_ is intended only for Windows!")
load(qt_module_config)
DESTDIR = $$QT.core.libs
wince*:QMAKE_POST_LINK =


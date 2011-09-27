TEMPLATE = lib
CONFIG += dll
CONFIG -= staticlib
SOURCES		= mylib.c
TARGET = mylib
DESTDIR = ../
VERSION = 2
QT = core

wince*: DEFINES += WIN32_MSVC
win32-msvc: DEFINES += WIN32_MSVC
win32-borland: DEFINES += WIN32_BORLAND

# Force a copy of the library to have an extension that is non-standard.
# We want to test if we can load a shared library with *any* filename...

win32 {
    # vcproj and Makefile generators refer to target differently
    contains(TEMPLATE,vc.*) {
        src = $(TargetPath)
    } else {
        src = $(DESTDIR_TARGET)
    }
    files = mylib.dl2 system.trolltech.test.mylib.dll
} else {
    src = $(DESTDIR)$(TARGET)
    files = libmylib.so2 system.trolltech.test.mylib.so
}
for(file, files) {
    QMAKE_POST_LINK += $$QMAKE_COPY $$src ..$$QMAKE_DIR_SEP$$file &&
    CLEAN_FILES += ../$$file
}
QMAKE_POST_LINK = $$member(QMAKE_POST_LINK, 0, -2)

#no special install rule for the library used by test
INSTALLS =



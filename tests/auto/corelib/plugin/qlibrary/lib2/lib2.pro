TEMPLATE = lib
CONFIG += dll
CONFIG -= staticlib
SOURCES		= mylib.c
TARGET = mylib
DESTDIR = ../
VERSION = 2
QT = core

msvc: DEFINES += WIN32_MSVC

# Force a copy of the library to have an extension that is non-standard.
# We want to test if we can load a shared library with *any* filename...

win32 {

    debug_and_release {
        CONFIG(debug, debug|release)) {
            BUILD_FOLDER = debug
        } else {
            BUILD_FOLDER = release
        }
        DESTDIR = ../$$BUILD_FOLDER/
    } else {
        BUILD_FOLDER =
        DESTDIR = ../
    }

    # vcproj and Makefile generators refer to target differently
    contains(TEMPLATE,vc.*) {
        src = $(TargetPath)
    } else {
        src = $(DESTDIR_TARGET)
    }
    files = $$BUILD_FOLDER$${QMAKE_DIR_SEP}mylib.dl2 $$BUILD_FOLDER$${QMAKE_DIR_SEP}system.qt.test.mylib.dll
} else {
    src = $(DESTDIR)$(TARGET)
    files = libmylib.so2 system.qt.test.mylib.so
}

# This project is testdata for tst_qlibrary
target.path = $$[QT_INSTALL_TESTS]$${QMAKE_DIR_SEP}tst_qlibrary
renamed_target.path = $$target.path

for(file, files) {
    QMAKE_POST_LINK += $$QMAKE_COPY $$src ..$$QMAKE_DIR_SEP$$file &&
    renamed_target.extra += $$QMAKE_COPY $$src $(INSTALL_ROOT)$${target.path}$$QMAKE_DIR_SEP$$file &&
    CLEAN_FILES += ..$$QMAKE_DIR_SEP$$file
}
renamed_target.extra = $$member(renamed_target.extra, 0, -2)
QMAKE_POST_LINK = $$member(QMAKE_POST_LINK, 0, -2)

INSTALLS += target renamed_target

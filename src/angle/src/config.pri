# This file contains build options that are relevant for both the compilers
# and the khronos implementation libraries.

ANGLE_DIR = $$(ANGLE_DIR)
isEmpty(ANGLE_DIR) {
    ANGLE_DIR = $$PWD/../../3rdparty/angle
} else {
    !build_pass:message("Using external ANGLE from $$ANGLE_DIR")
}

!exists($$ANGLE_DIR/src) {
    error("$$ANGLE_DIR does not contain ANGLE")
}

win32 {
    GNUTOOLS_DIR=$$PWD/../../../../gnuwin32/bin
    exists($$GNUTOOLS_DIR/gperf.exe) {
        GNUTOOLS = "(set $$escape_expand(\\\")PATH=$$replace(GNUTOOLS_DIR, [/\\\\], $${QMAKE_DIR_SEP});%PATH%$$escape_expand(\\\"))"
    }
}

defineReplace(addGnuPath) {
    unset(gnuPath)
    gnuPath = $$1
    !isEmpty(gnuPath):!isEmpty(GNUTOOLS) {
        eval(gnuPath = $${GNUTOOLS} && $$gnuPath)
        silent: eval(gnuPath = @echo generating sources from ${QMAKE_FILE_IN} && $$val_escape($$gnuPath))
    }
    return($$gnuPath)
}

# Defines for modifying Win32 headers
DEFINES +=  _WINDOWS \
            _UNICODE \
            _CRT_SECURE_NO_DEPRECATE \
            _HAS_EXCEPTIONS=0 \
            NOMINMAX \
            WIN32_LEAN_AND_MEAN=1

# Defines specifying the API version (0x0600 = Vista)
DEFINES +=  _WIN32_WINNT=0x0600 WINVER=0x0600

# ANGLE specific defines
DEFINES +=  ANGLE_DISABLE_TRACE \
            ANGLE_DISABLE_PERF \
            ANGLE_COMPILE_OPTIMIZATION_LEVEL=D3DCOMPILE_OPTIMIZATION_LEVEL0 \
            ANGLE_USE_NEW_PREPROCESSOR=1

# Force release builds for now. Debug builds of ANGLE will generate libraries with
# the 'd' library suffix, but this means that the library name no longer matches that
# listed in the DEF file which causes errors at runtime. Using the DEF is mandatory
# to generate the import library because the symbols are not marked with __declspec
# and therefore not exported by default. With the import library, the debug build is
# useless, so just disable until we can find another solution.
CONFIG -= debug
CONFIG += release

TARGET = $$qtLibraryTarget($$TARGET)

CONFIG(debug, debug|release) {
    DEFINES += _DEBUG
} else {
    DEFINES += NDEBUG
}

# c++11 is needed by MinGW to get support for unordered_map.
CONFIG -= qt
CONFIG += stl rtti_off exceptions c++11

INCLUDEPATH += . .. $$PWD/../include

DESTDIR = $$QT_BUILD_TREE/lib
DLLDESTDIR = $$QT_BUILD_TREE/bin

msvc {
    # Disabled Warnings:
    #   4100: 'identifier' : unreferenced formal parameter
    #   4127: conditional expression is constant
    #   4189: 'identifier' : local variable is initialized but not referenced
    #   4239: nonstandard extension used : 'token' : conversion from 'type' to 'type'
    #   4244: 'argument' : conversion from 'type1' to 'type2', possible loss of data
    #   4245: 'conversion' : conversion from 'type1' to 'type2', signed/unsigned mismatch
    #   4512: 'class' : assignment operator could not be generated
    #   4702: unreachable code
    QMAKE_CFLAGS_WARN_ON    = -W4 -wd"4100" -wd"4127" -wd"4189" -wd"4239" -wd"4244" -wd"4245" -wd"4512" -wd"4702"
    QMAKE_CFLAGS_RELEASE    = -O2 -Oy- -MT  -Gy -GS -Gm-
    QMAKE_CFLAGS_DEBUG      = -Od -Oy- -MTd -Gy -GS -Gm- -RTC1
    QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO = -Zi $$QMAKE_CFLAGS_RELEASE

    QMAKE_CXXFLAGS_WARN_ON = $$QMAKE_CFLAGS_WARN_ON
}

gcc {
    QMAKE_CFLAGS_WARN_ON =  -Wall -Wno-unknown-pragmas -Wno-comment -Wno-missing-field-initializers \
                            -Wno-switch -Wno-unused-parameter -Wno-write-strings -Wno-sign-compare -Wno-missing-braces \
                            -Wno-unused-but-set-variable -Wno-unused-variable -Wno-narrowing -Wno-maybe-uninitialized \
                            -Wno-strict-aliasing -Wno-type-limits

    QMAKE_CXXFLAGS_WARN_ON = $$QMAKE_CFLAGS_WARN_ON -Wno-reorder -Wno-conversion-null -Wno-delete-non-virtual-dtor
}

QMAKE_CXXFLAGS_DEBUG = $$QMAKE_CFLAGS_DEBUG
QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE

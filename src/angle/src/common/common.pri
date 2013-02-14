include (../config.pri)

INCLUDEPATH += \
    $$ANGLE_DIR/src \
    $$ANGLE_DIR/include

LIBS = $$QMAKE_LIBS_CORE $$QMAKE_LIBS_GUI

# DirectX is included in the Windows 8 Kit, but everything else requires the DX SDK.
win32-msvc2012 {
    FXC = fxc.exe
} else {
    DX_DIR = $$(DXSDK_DIR)
    isEmpty(DX_DIR) {
        error("Cannot determine DirectX SDK location. Please set DXSDK_DIR environment variable.")
    }

    DXINC_DIR = $${DX_DIR}Include
    contains(QT_ARCH, x86_64) {
        DXLIB_DIR = $${DX_DIR}Lib\\x64
    } else {
        DXLIB_DIR = $${DX_DIR}Lib\\x86
    }

    equals(QMAKE_TARGET.arch, x86_64) {
        FXC = \"$${DX_DIR}Utilities\\bin\\x64\\fxc.exe\"
    } else {
        FXC = \"$${DX_DIR}Utilities\\bin\\x86\\fxc.exe\"
    }

    msvc {
        # Unfortunately MinGW cannot use the DirectX headers from the DX SDK because d3d11shader.h uses
        # buffer annotation macros (eg: __out, __in) which are not defined in the MinGW copy of
        # specstrings_strict.h
        INCLUDEPATH += $$DXINC_DIR

        # Similarly we want the MinGW linker to use the import libraries shipped with the compiler
        # instead of those from the SDK which cause a crash on startup.
        LIBS += -L\"$$DXLIB_DIR\"
    }
}

static: DEFINES *= QT_OPENGL_ES_2_ANGLE_STATIC

HEADERS +=  \
    $$ANGLE_DIR/src/common/angleutils.h \
    $$ANGLE_DIR/src/common/debug.h \
    $$ANGLE_DIR/src/common/RefCountObject.h \
    $$ANGLE_DIR/src/common/version.h

SOURCES +=  \
    $$ANGLE_DIR/src/common/debug.cpp \
    $$ANGLE_DIR/src/common/RefCountObject.cpp


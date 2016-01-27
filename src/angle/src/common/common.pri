CONFIG += installed
include (../config.pri)

INCLUDEPATH += \
    $$ANGLE_DIR/src \
    $$ANGLE_DIR/include

LIBS_PRIVATE = $$QMAKE_LIBS_CORE $$QMAKE_LIBS_GUI

TR_EXCLUDE += $$ANGLE_DIR/src/*

lib_replace.match = $$[QT_INSTALL_LIBS/get]
lib_replace.replace = \$\$\$\$[QT_INSTALL_LIBS]
lib_replace.CONFIG = path
QMAKE_PRL_INSTALL_REPLACE += lib_replace

# DirectX is included in the Windows 8 Kit, but everything else requires the DX SDK.
winrt|if(msvc:!win32-msvc2005:!win32-msvc2008:!win32-msvc2010) {
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
        LIBS_PRIVATE += -L$$DXLIB_DIR
    }
}

# static builds should still link ANGLE dynamically when dynamic GL is enabled
static:contains(QT_CONFIG, dynamicgl) {
    CONFIG -= static
    CONFIG += shared
}

static: DEFINES *= LIBGLESV2_EXPORT_H_ ANGLE_EXPORT=

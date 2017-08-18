# static builds should still link ANGLE dynamically when dynamic GL is enabled
include($$OUT_PWD/../../../gui/qtgui-config.pri)
static:qtConfig(dynamicgl) {
    CONFIG -= static
    CONFIG += shared
}

CONFIG += installed
include (../config.pri)

INCLUDEPATH += \
    $$ANGLE_DIR/src \
    $$ANGLE_DIR/include

LIBS_PRIVATE = $$QMAKE_LIBS_GUI

TR_EXCLUDE += $$ANGLE_DIR/src/*

lib_replace.match = $$[QT_INSTALL_LIBS/get]
lib_replace.replace = \$\$\$\$[QT_INSTALL_LIBS]
lib_replace.CONFIG = path
QMAKE_PRL_INSTALL_REPLACE += lib_replace

# DirectX is included in the Windows 8 Kit, but everything else requires the DX SDK.
winrt|msvc {
    FXC = fxc.exe
} else {
    DX_DIR = $$(DXSDK_DIR)
    isEmpty(DX_DIR) {
        error("Cannot determine DirectX SDK location. Please set DXSDK_DIR environment variable.")
    }

    equals(QMAKE_TARGET.arch, x86_64) {
        FXC = \"$${DX_DIR}Utilities\\bin\\x64\\fxc.exe\"
    } else {
        FXC = \"$${DX_DIR}Utilities\\bin\\x86\\fxc.exe\"
    }
}

static: DEFINES *= LIBGLESV2_EXPORT_H_ ANGLE_EXPORT=

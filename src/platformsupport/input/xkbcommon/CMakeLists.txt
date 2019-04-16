# Generated from xkbcommon.pro.

#####################################################################
## XkbCommonSupport Module:
#####################################################################

add_qt_module(XkbCommonSupport
    STATIC
    SOURCES
        qxkbcommon.cpp qxkbcommon_p.h
        qxkbcommon_3rdparty.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        XKB::XKB
)

#### Keys ignored in scope 1:.:.:xkbcommon.pro:<TRUE>:
# CONFIG = "static" "internal_module"
# MODULE = "xkbcommon_support"
# PRECOMPILED_HEADER = "../../../corelib/global/qt_pch.h"
# _LOADED = "qt_module"

## Scopes:
#####################################################################

extend_target(XkbCommonSupport CONDITION CLANG AND NOT ICC
    COMPILE_OPTIONS
        "-ftemplate-depth=1024"
)

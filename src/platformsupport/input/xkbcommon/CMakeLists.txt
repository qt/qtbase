# Generated from xkbcommon.pro.

#####################################################################
## XkbCommonSupport Module:
#####################################################################

add_qt_module(XkbCommonSupport
    STATIC
    INTERNAL_MODULE
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
    PRECOMPILED_HEADER
        "../../../corelib/global/qt_pch.h"
)

#### Keys ignored in scope 1:.:.:xkbcommon.pro:<TRUE>:
# MODULE = "xkbcommon_support"
# _LOADED = "qt_module"

## Scopes:
#####################################################################

extend_target(XkbCommonSupport CONDITION CLANG AND NOT ICC
    COMPILE_OPTIONS
        -ftemplate-depth=1024
)

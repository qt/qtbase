# Generated from xkbcommon.pro.

#####################################################################
## XkbCommonSupport Module:
#####################################################################

qt_add_module(XkbCommonSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        qxkbcommon.cpp qxkbcommon_p.h
        qxkbcommon_3rdparty.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        XKB::XKB
    PRECOMPILED_HEADER
        "../../../corelib/global/qt_pch.h"
)

#### Keys ignored in scope 1:.:.:xkbcommon.pro:<TRUE>:
# MODULE = "xkbcommon_support"

## Scopes:
#####################################################################

qt_extend_target(XkbCommonSupport CONDITION CLANG AND NOT ICC
    COMPILE_OPTIONS
        -ftemplate-depth=1024
)

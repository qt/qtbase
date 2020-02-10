# Generated from edid.pro.

#####################################################################
## EdidSupport Module:
#####################################################################

qt_add_module(EdidSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        qedidparser.cpp qedidparser_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Qt::CorePrivate
    PRECOMPILED_HEADER
        "../../corelib/global/qt_pch.h"
)

#### Keys ignored in scope 1:.:.:edid.pro:<TRUE>:
# MODULE = "edid_support"

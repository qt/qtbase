# Generated from fbconvenience.pro.

#####################################################################
## FbSupport Module:
#####################################################################

add_qt_module(FbSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        qfbbackingstore.cpp qfbbackingstore_p.h
        qfbcursor.cpp qfbcursor_p.h
        qfbscreen.cpp qfbscreen_p.h
        qfbvthandler.cpp qfbvthandler_p.h
        qfbwindow.cpp qfbwindow_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
    PRECOMPILED_HEADER
        "../../corelib/global/qt_pch.h"
)

#### Keys ignored in scope 1:.:.:fbconvenience.pro:<TRUE>:
# CONFIG = "static" "internal_module"
# MODULE = "fb_support"
# _LOADED = "qt_module"

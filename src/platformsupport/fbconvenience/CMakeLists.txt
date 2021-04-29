# Generated from fbconvenience.pro.

#####################################################################
## FbSupportPrivate Module:
#####################################################################

qt_internal_add_module(FbSupportPrivate
    CONFIG_MODULE_NAME fb_support
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
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PRECOMPILED_HEADER
        "../../corelib/global/qt_pch.h"
)

#### Keys ignored in scope 1:.:.:fbconvenience.pro:<TRUE>:
# MODULE = "fb_support"

# Generated from platformheaders.pro.

#####################################################################
## PlatformHeaders Module:
#####################################################################

qt_add_module(PlatformHeaders
    HEADER_MODULE
    # special case begin
    # SOURCES
    #     eglfsfunctions/qeglfsfunctions_p.h
    #     helper/qplatformheaderhelper_p.h
    #     linuxfbfunctions/qlinuxfbfunctions_p.h
    #     waylandfunctions/qwaylandwindowfunctions_p.h
    #     windowsfunctions/qwindowswindowfunctions_p.h
    # special case end
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
)
qt_add_docs(PlatformHeaders
    doc/qtplatformheaders.qdocconf
)


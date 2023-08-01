qt_internal_add_manual_test(hdr
    GUI
    SOURCES
        hdr.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

qt_internal_add_resource(hdr "hdr"
    PREFIX
        "/"
    FILES
        "hdrtexture.vert.qsb"
        "hdrtexture.frag.qsb"
)

set(imgui_base ../shared/imgui)
set(imgui_target hdr)
include(${imgui_base}/imgui.cmakeinc)

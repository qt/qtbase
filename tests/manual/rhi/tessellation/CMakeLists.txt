qt_internal_add_manual_test(tessellation
    GUI
    SOURCES
        tessellation.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

set(tessellation_resource_files
    "test.vert.qsb"
    "test.tesc.qsb"
    "test.tese.qsb"
    "test.frag.qsb"
)

qt_internal_add_resource(tessellation "tessellation"
    PREFIX
        "/"
    FILES
        ${tessellation_resource_files}
)

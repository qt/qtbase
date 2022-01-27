qt_internal_add_manual_test(geometryshader
    GUI
    SOURCES
        geometryshader.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

set(geometryshader_resource_files
    "test.vert.qsb"
    "test.geom.qsb"
    "test.frag.qsb"
)

qt_internal_add_resource(geometryshader "geometryshader"
    PREFIX
        "/"
    FILES
        ${geometryshader_resource_files}
)

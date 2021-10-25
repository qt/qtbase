qt_internal_add_manual_test(rhiwidget
    GUI
    SOURCES
        examplewidget.cpp examplewidget.h
        rhiwidget.cpp rhiwidget.h rhiwidget_p.h
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

set_source_files_properties("../shared/texture.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.vert.qsb"
)
set_source_files_properties("../shared/texture.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.frag.qsb"
)
set(rhiwidget_resource_files
    "../shared/texture.vert.qsb"
    "../shared/texture.frag.qsb"
)

qt_internal_add_resource(rhiwidget "rhiwidget"
    PREFIX
        "/"
    FILES
        ${rhiwidget_resource_files}
)

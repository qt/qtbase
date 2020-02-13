# Generated from multiwindow.pro.

#####################################################################
## multiwindow Binary:
#####################################################################

add_qt_manual_test(multiwindow
    GUI
    SOURCES
        multiwindow.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)

# Resources:
set_source_files_properties("../shared/color.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.frag.qsb"
)
set_source_files_properties("../shared/color.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.vert.qsb"
)
set(multiwindow_resource_files
    "../shared/color.frag.qsb"
    "../shared/color.vert.qsb"
)

add_qt_resource(multiwindow "multiwindow"
    PREFIX
        "/"
    FILES
        ${multiwindow_resource_files}
)


#### Keys ignored in scope 1:.:.:multiwindow.pro:<TRUE>:
# TEMPLATE = "app"

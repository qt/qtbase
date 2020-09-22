# Generated from multiwindow_threaded.pro.

#####################################################################
## multiwindow_threaded Binary:
#####################################################################

qt_internal_add_manual_test(multiwindow_threaded
    GUI
    SOURCES
        multiwindow_threaded.cpp
        window.cpp window.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)

# Resources:
set_source_files_properties("../shared/qt256.png"
    PROPERTIES QT_RESOURCE_ALIAS "qt256.png"
)
set_source_files_properties("../shared/texture.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.frag.qsb"
)
set_source_files_properties("../shared/texture.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.vert.qsb"
)
set(multiwindow_threaded_resource_files
    "../shared/qt256.png"
    "../shared/texture.frag.qsb"
    "../shared/texture.vert.qsb"
)

qt_internal_add_resource(multiwindow_threaded "multiwindow_threaded"
    PREFIX
        "/"
    FILES
        ${multiwindow_threaded_resource_files}
)


#### Keys ignored in scope 1:.:.:multiwindow_threaded.pro:<TRUE>:
# TEMPLATE = "app"

# Generated from grab_override.pro.

#####################################################################
## t_cursors Binary:
#####################################################################

qt_internal_add_manual_test(t_cursors
    GUI
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h mainwindow.ui
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

# Resources:
set(images_resource_files
    "data/monkey_on_64x64.png"
)

qt_internal_add_resource(t_cursors "images"
    PREFIX
        "/"
    FILES
        ${images_resource_files}
)


#### Keys ignored in scope 1:.:.:grab_override.pro:<TRUE>:
# TEMPLATE = "app"

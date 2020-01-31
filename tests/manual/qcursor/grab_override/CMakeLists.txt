# Generated from grab_override.pro.

#####################################################################
## t_cursors Binary:
#####################################################################

add_qt_manual_test(t_cursors
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

add_qt_resource(t_cursors "images"
    PREFIX
        "/"
    FILES
        ${images_resource_files}
)


#### Keys ignored in scope 1:.:.:grab_override.pro:<TRUE>:
# TEMPLATE = "app"

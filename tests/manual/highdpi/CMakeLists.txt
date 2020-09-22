# Generated from highdpi.pro.

#####################################################################
## highdpi Binary:
#####################################################################

qt_internal_add_manual_test(highdpi
    SOURCES
        dragwidget.cpp dragwidget.h
        main.cpp
    DEFINES
        HAVE_SCREEN_BASE_DPI
    INCLUDE_DIRECTORIES
        .
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)

# Resources:
set(highdpi_resource_files
    "qticon16.png"
    "qticon16@2x.png"
    "qticon16@3x.png"
    "qticon32.png"
    "qticon32@2x.png"
    "qticon64.png"
)

qt_internal_add_resource(highdpi "highdpi"
    PREFIX
        "/"
    FILES
        ${highdpi_resource_files}
)


#### Keys ignored in scope 1:.:.:highdpi.pro:<TRUE>:
# TEMPLATE = "app"

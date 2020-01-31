# Generated from highdpi.pro.

#####################################################################
## highdpi Binary:
#####################################################################

add_qt_manual_test(highdpi
    SOURCES
        dragwidget.cpp dragwidget.h
        main.cpp
    DEFINES
        HAVE_SCREEN_BASE_DPI
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
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

add_qt_resource(highdpi "highdpi"
    PREFIX
        "/"
    FILES
        ${highdpi_resource_files}
)


#### Keys ignored in scope 1:.:.:highdpi.pro:<TRUE>:
# TEMPLATE = "app"

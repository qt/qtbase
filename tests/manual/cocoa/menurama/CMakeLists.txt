# Generated from menurama.pro.

#####################################################################
## Menurama Binary:
#####################################################################

qt_internal_add_manual_test(Menurama
    GUI
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h mainwindow.ui
        menuramaapplication.cpp menuramaapplication.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

#### Keys ignored in scope 1:.:.:menurama.pro:<TRUE>:
# TEMPLATE = "app"

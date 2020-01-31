# Generated from bearerex.pro.

#####################################################################
## BearerEx Binary:
#####################################################################

add_qt_manual_test(BearerEx
    GUI
    SOURCES
        bearerex.cpp bearerex.h bearerex.ui
        datatransferer.cpp datatransferer.h
        detailedinfodialog.ui
        main.cpp
        sessiondialog.ui
        xqlistwidget.cpp xqlistwidget.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

#### Keys ignored in scope 1:.:.:bearerex.pro:<TRUE>:
# TEMPLATE = "app"

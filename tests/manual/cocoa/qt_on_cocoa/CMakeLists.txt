# Generated from qt_on_cocoa.pro.

#####################################################################
## qt_on_cocoa Binary:
#####################################################################

qt_internal_add_manual_test(qt_on_cocoa
    GUI
    SOURCES
        main.mm
        rasterwindow.cpp rasterwindow.h
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    PUBLIC_LIBRARIES
        ${FWAppKit}
        Qt::Gui
        Qt::Quick
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:qt_on_cocoa.pro:<TRUE>:
# TEMPLATE = "app"

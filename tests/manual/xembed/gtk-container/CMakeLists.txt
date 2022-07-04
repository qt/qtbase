# Generated from gtk-container.pro.

qt_find_package(GTK3) # special case
qt_find_package(X11) # special case

#####################################################################
## gtk-container Binary:
#####################################################################

qt_internal_add_manual_test(gtk-container
    GUI
    SOURCES
        gtk-container.cpp
        DEFINES
        GDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_6
    LIBRARIES # special case
        PkgConfig::GTK3
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        X11::X11 # special case
)

#### Keys ignored in scope 1:.:.:qt-client-widget.pro:<TRUE>:
# TEMPLATE = "app"


# Generated from flattreeview.pro.

#####################################################################
## flattreeview Binary:
#####################################################################

add_qt_executable(flattreeview
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/flattreeview"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/flattreeview"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Widgets
)

# Generated from storageview.pro.

#####################################################################
## storageview Binary:
#####################################################################

add_qt_executable(storageview
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/storageview"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/storageview"
    SOURCES
        main.cpp
        storagemodel.cpp storagemodel.h
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)

#### Keys ignored in scope 1:.:storageview.pro:<NONE>:
# TEMPLATE = "app"

# Generated from editabletreemodel.pro.

#####################################################################
## editabletreemodel Binary:
#####################################################################

add_qt_executable(editabletreemodel
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/editabletreemodel"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/editabletreemodel"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h mainwindow.ui
        treeitem.cpp treeitem.h
        treemodel.cpp treemodel.h
    PUBLIC_LIBRARIES
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

# Resources:
add_qt_resource(editabletreemodel "editabletreemodel" PREFIX "/" FILES
    default.txt)


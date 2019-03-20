# Generated from simpletreemodel.pro.

#####################################################################
## simpletreemodel Binary:
#####################################################################

add_qt_executable(simpletreemodel
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/simpletreemodel"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/simpletreemodel"
    SOURCES
        main.cpp
        treeitem.cpp treeitem.h
        treemodel.cpp treemodel.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(simpletreemodel "simpletreemodel" FILES
    default.txt)


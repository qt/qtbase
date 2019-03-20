# Generated from treemodelcompleter.pro.

#####################################################################
## treemodelcompleter Binary:
#####################################################################

add_qt_executable(treemodelcompleter
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/treemodelcompleter"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/treemodelcompleter"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        treemodelcompleter.cpp treemodelcompleter.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(treemodelcompleter "treemodelcompleter" PREFIX "/" FILES
    resources/treemodel.txt)


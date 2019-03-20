# Generated from elasticnodes.pro.

#####################################################################
## elasticnodes Binary:
#####################################################################

add_qt_executable(elasticnodes
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/elasticnodes"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/elasticnodes"
    SOURCES
        edge.cpp edge.h
        graphwidget.cpp graphwidget.h
        main.cpp
        node.cpp node.h
    LIBRARIES
        Qt::Widgets
)

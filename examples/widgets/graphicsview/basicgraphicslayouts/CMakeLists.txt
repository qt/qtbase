# Generated from basicgraphicslayouts.pro.

#####################################################################
## basicgraphicslayouts Binary:
#####################################################################

add_qt_executable(basicgraphicslayouts
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/basicgraphicslayouts"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/basicgraphicslayouts"
    SOURCES
        layoutitem.cpp layoutitem.h
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(basicgraphicslayouts "basicgraphicslayouts" FILES
    images/block.png)


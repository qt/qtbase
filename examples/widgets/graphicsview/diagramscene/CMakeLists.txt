# Generated from diagramscene.pro.

#####################################################################
## diagramscene Binary:
#####################################################################

add_qt_executable(diagramscene
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/diagramscene"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/diagramscene"
    SOURCES
        arrow.cpp arrow.h
        diagramitem.cpp diagramitem.h
        diagramscene.cpp diagramscene.h
        diagramtextitem.cpp diagramtextitem.h
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(diagramscene "diagramscene" FILES
    images/background1.png
    images/background2.png
    images/background3.png
    images/background4.png
    images/bold.png
    images/bringtofront.png
    images/delete.png
    images/floodfill.png
    images/italic.png
    images/linecolor.png
    images/linepointer.png
    images/pointer.png
    images/sendtoback.png
    images/textpointer.png
    images/underline.png)


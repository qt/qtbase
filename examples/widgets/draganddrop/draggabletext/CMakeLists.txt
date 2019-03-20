# Generated from draggabletext.pro.

#####################################################################
## draggabletext Binary:
#####################################################################

add_qt_executable(draggabletext
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/draganddrop/draggabletext"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/draganddrop/draggabletext"
    SOURCES
        dragwidget.cpp dragwidget.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(draggabletext "draggabletext" PREFIX "/dictionary" FILES
    words.txt)


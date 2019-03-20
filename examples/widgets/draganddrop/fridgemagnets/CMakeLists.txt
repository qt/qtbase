# Generated from fridgemagnets.pro.

#####################################################################
## fridgemagnets Binary:
#####################################################################

add_qt_executable(fridgemagnets
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/draganddrop/fridgemagnets"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/draganddrop/fridgemagnets"
    SOURCES
        draglabel.cpp draglabel.h
        dragwidget.cpp dragwidget.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(fridgemagnets "fridgemagnets" PREFIX "/dictionary" FILES
    words.txt)


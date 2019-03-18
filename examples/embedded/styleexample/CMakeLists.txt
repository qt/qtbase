# Generated from styleexample.pro.

#####################################################################
## styleexample Binary:
#####################################################################

add_qt_executable(styleexample
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/embedded/styleexample"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/embedded/styleexample"
    SOURCES
        main.cpp
        stylewidget.cpp stylewidget.h stylewidget.ui
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(styleexample "styleexample" PREFIX "/" FILES
    files/add.png
    files/application.qss
    files/blue.qss
    files/khaki.qss
    files/nature_1.jpg
    files/nostyle.qss
    files/remove.png
    files/transparent.qss)


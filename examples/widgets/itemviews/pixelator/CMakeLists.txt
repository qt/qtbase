# Generated from pixelator.pro.

#####################################################################
## pixelator Binary:
#####################################################################

add_qt_executable(pixelator
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/pixelator"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/pixelator"
    SOURCES
        imagemodel.cpp imagemodel.h
        main.cpp
        mainwindow.cpp mainwindow.h
        pixeldelegate.cpp pixeldelegate.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(pixelator "images" FILES
    images/qt.png)


## Scopes:
#####################################################################

extend_target(pixelator CONDITION TARGET Qt::PrintSupport
    LIBRARIES
        Qt::PrintSupport
)

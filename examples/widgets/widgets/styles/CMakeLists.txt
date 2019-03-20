# Generated from styles.pro.

#####################################################################
## styles Binary:
#####################################################################

add_qt_executable(styles
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/styles"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/styles"
    SOURCES
        main.cpp
        norwegianwoodstyle.cpp norwegianwoodstyle.h
        widgetgallery.cpp widgetgallery.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(styles "styles" FILES
    images/woodbackground.png
    images/woodbutton.png)


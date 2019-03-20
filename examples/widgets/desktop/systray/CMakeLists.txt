# Generated from systray.pro.

#####################################################################
## systray Binary:
#####################################################################

add_qt_executable(systray
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/desktop/systray"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/desktop/systray"
    SOURCES
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(systray "systray" PREFIX "/" FILES
    images/bad.png
    images/heart.png
    images/trash.png)


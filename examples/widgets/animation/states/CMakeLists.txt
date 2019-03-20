# Generated from states.pro.

#####################################################################
## states Binary:
#####################################################################

add_qt_executable(states
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/animation/states"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/animation/states"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(states "states" FILES
    accessories-dictionary.png
    akregator.png
    digikam.png
    help-browser.png
    k3b.png
    kchart.png)


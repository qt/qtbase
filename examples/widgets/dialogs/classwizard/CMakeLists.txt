# Generated from classwizard.pro.

#####################################################################
## classwizard Binary:
#####################################################################

add_qt_executable(classwizard
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/dialogs/classwizard"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/dialogs/classwizard"
    SOURCES
        classwizard.cpp classwizard.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(classwizard "classwizard" FILES
    images/background.png
    images/banner.png
    images/logo1.png
    images/logo2.png
    images/logo3.png
    images/watermark1.png
    images/watermark2.png)


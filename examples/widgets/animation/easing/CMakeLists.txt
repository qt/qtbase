# Generated from easing.pro.

#####################################################################
## easing Binary:
#####################################################################

add_qt_executable(easing
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/animation/easing"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/animation/easing"
    SOURCES
        animation.h
        form.ui
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(easing "easing" FILES
    images/qt-logo.png)


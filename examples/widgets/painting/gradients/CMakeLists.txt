# Generated from gradients.pro.

#####################################################################
## gradients Binary:
#####################################################################

add_qt_executable(gradients
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/gradients"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/gradients"
    SOURCES
        gradients.cpp gradients.h
        main.cpp
    LIBRARIES # special case
        painting_shared
)

# Resources:
add_qt_resource(gradients "gradients" PREFIX "/res/gradients" FILES
    gradients.cpp
    gradients.html)

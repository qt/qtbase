# Generated from composition.pro.

#####################################################################
## composition Binary:
#####################################################################

add_qt_executable(composition
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/composition"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/composition"
    SOURCES
        composition.cpp composition.h
        main.cpp
    LIBRARIES # special case
        painting_shared
)

# Resources:
add_qt_resource(composition "composition" PREFIX "/res/composition" FILES
    composition.cpp
    composition.html
    flower.jpg
    flower_alpha.jpg)

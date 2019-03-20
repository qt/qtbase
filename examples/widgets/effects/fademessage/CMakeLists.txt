# Generated from fademessage.pro.

#####################################################################
## fademessage Binary:
#####################################################################

add_qt_executable(fademessage
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/effects/fademessage"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/effects/fademessage"
    SOURCES
        fademessage.cpp fademessage.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(fademessage "fademessage" PREFIX "/" FILES
    background.jpg)


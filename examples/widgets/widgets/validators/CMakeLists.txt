# Generated from validators.pro.

#####################################################################
## validators Binary:
#####################################################################

add_qt_executable(validators
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/validators"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/validators"
    SOURCES
        ledwidget.cpp ledwidget.h
        localeselector.cpp localeselector.h
        main.cpp
        validators.ui
        validatorwidget.cpp validatorwidget.h
    PUBLIC_LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(validators "validators" PREFIX "/" FILES
    ledoff.png
    ledon.png)


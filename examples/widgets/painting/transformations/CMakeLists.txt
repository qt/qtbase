# Generated from transformations.pro.

#####################################################################
## transformations Binary:
#####################################################################

add_qt_executable(transformations
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/transformations"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/transformations"
    SOURCES
        main.cpp
        renderarea.cpp renderarea.h
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)

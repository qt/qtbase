# Generated from sliders.pro.

#####################################################################
## sliders Binary:
#####################################################################

add_qt_executable(sliders
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/sliders"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/sliders"
    SOURCES
        main.cpp
        slidersgroup.cpp slidersgroup.h
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)

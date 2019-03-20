# Generated from tetrix.pro.

#####################################################################
## tetrix Binary:
#####################################################################

add_qt_executable(tetrix
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/tetrix"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/tetrix"
    SOURCES
        main.cpp
        tetrixboard.cpp tetrixboard.h
        tetrixpiece.cpp tetrixpiece.h
        tetrixwindow.cpp tetrixwindow.h
    LIBRARIES
        Qt::Widgets
)

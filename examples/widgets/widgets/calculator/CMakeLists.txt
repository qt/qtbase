# Generated from calculator.pro.

#####################################################################
## calculator Binary:
#####################################################################

add_qt_executable(calculator
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/calculator"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/calculator"
    SOURCES
        button.cpp button.h
        calculator.cpp calculator.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

# Generated from coloreditorfactory.pro.

#####################################################################
## coloreditorfactory Binary:
#####################################################################

add_qt_executable(coloreditorfactory
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/coloreditorfactory"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/coloreditorfactory"
    SOURCES
        colorlisteditor.cpp colorlisteditor.h
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)

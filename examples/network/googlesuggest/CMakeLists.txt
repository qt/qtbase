# Generated from googlesuggest.pro.

#####################################################################
## googlesuggest Binary:
#####################################################################

add_qt_executable(googlesuggest
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/googlesuggest"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/googlesuggest"
    SOURCES
        googlesuggest.cpp googlesuggest.h
        main.cpp
        searchbox.cpp searchbox.h
    LIBRARIES
        Qt::Network
        Qt::Widgets
)

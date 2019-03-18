# Generated from digiflip.pro.

#####################################################################
## digiflip Binary:
#####################################################################

add_qt_executable(digiflip
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/embedded/digiflip"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/embedded/digiflip"
    SOURCES
        digiflip.cpp
    LIBRARIES
        Qt::Widgets
)

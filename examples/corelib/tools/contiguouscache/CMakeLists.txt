# Generated from contiguouscache.pro.

#####################################################################
## contiguouscache Binary:
#####################################################################

add_qt_executable(contiguouscache
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/tools/contiguouscache"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/tools/contiguouscache"
    SOURCES
        main.cpp
        randomlistmodel.cpp randomlistmodel.h
    LIBRARIES
        Qt::Widgets
)

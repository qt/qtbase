# Generated from customtypesending.pro.

#####################################################################
## customtypesending Binary:
#####################################################################

add_qt_executable(customtypesending
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/tools/customtypesending"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/tools/customtypesending"
    SOURCES
        main.cpp
        message.cpp message.h
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)

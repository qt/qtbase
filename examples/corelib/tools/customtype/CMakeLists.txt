# Generated from customtype.pro.

#####################################################################
## customtype Binary:
#####################################################################

add_qt_executable(customtype
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/tools/customtype"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/tools/customtype"
    SOURCES
        main.cpp
        message.cpp message.h
    LIBRARIES
        Qt::Widgets
)
